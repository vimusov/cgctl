// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <libgen.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sendfile.h>

#include "log.h"
#include "utils.h"

uint64_t str2uint(const char *const value)
{
    assert(value != NULL);

    // Фильтруем пустые и отрицательные значения.
    if (*value == '\0' || *value == '-' || *value == '\n')
        return 0;

    char *endptr;

    const uint64_t ret = strtoull(value, &endptr, 10);

    if (*endptr == '\0' || *endptr == '\n')
        return ret; // Валидное значение.
    else
        return 0; // Ошибка.
}

unsigned int get_cpu_usage(const char *const value)
{
    const unsigned int ret = str2uint(value);

    if (ret < 1 || ret > 100)
        errx(EXIT_FAILURE, "Invalid CPU usage percent value '%s', must be in [1..100].", value);

    return ret;
}

unsigned int get_mem_usage(const char *const value)
{
    const unsigned int ret = str2uint(value);

    if (ret < 1 || ret > 100)
        errx(EXIT_FAILURE, "Invalid memory usage percent value '%s', must be in [1..100].", value);

    return ret;
}

void format_path(char *file_path, const char *const dir_path, const char *const entry_name)
{
    if (snprintf(file_path, MAX_FILE_PATH, "%s/%s", dir_path, entry_name) <= 1) {
        LOG_C("Unable to format path from '%s'/'%s', error '%m'.", dir_path, entry_name);
        abort();
    }
}

int read_num(uint64_t *out_value, const char *const dir_path, const char *const file_name)
{
    char file_path[MAX_FILE_PATH];

    format_path(file_path, dir_path, file_name);

    FILE *const fp = fopen(file_path, "re");

    if (fp == NULL) {
        LOG_E("Unable to open file '%s', error '%m'.", file_path);
        return 1;
    }

    setbuf(fp, NULL);

    int exit_code = 1;
    char buf[MAX_UINT64_STR_SIZE];

    if (fgets(buf, sizeof(buf), fp) == NULL) {
        LOG_E("Unable to read line from file '%s', error '%m'.", file_path);

    } else {
        *out_value = str2uint(buf);
        LOG_D("Got value %" PRIu64 " from '%s'.", *out_value, file_path);
        exit_code = 0;
    }

    if (fclose(fp) == EOF) {
        LOG_C("Unable to close file '%s', error '%m'.", file_path);
        abort();
    }

    return exit_code;
}

int write_num(const uint64_t value, const char *const dir_path, const char *const file_name)
{
    char file_path[MAX_FILE_PATH];

    format_path(file_path, dir_path, file_name);

    FILE *const fp = fopen(file_path, "we");

    if (fp == NULL) {
        LOG_E("Unable to open file '%s', error '%m'.", file_path);
        return 1;
    }

    setbuf(fp, NULL);

    int exit_code = 1;

    LOG_D("Writing value %" PRIu64 " to '%s'.", value, file_path);

    if (fprintf(fp, "%" PRIu64 "\n", value) <= 0)
        LOG_E("Unable to write value %" PRIu64 " to file '%s', error '%m'.", value, file_path);
    else
        exit_code = 0;

    if (fclose(fp) == EOF) {
        LOG_C("Unable to close file '%s', error '%m'.", file_path);
        abort();
    }

    return exit_code;
}

void get_group_name(const char *const file_path, char *group)
{
    assert(file_path != NULL);
    assert(group != NULL);

    char abs_path[PATH_MAX + 1];

    if (realpath(file_path, abs_path) == NULL) {
        if (errno == ENOENT || errno == ENOTDIR) {
            /*
             * Если файл не найден или первый элемент пути не является каталогом,
             * то берём название группы от заданного пути.
             * Такое может быть например в upstart jobs где указывается только
             * название запускаемого бинарника без полного пути к нему.
             */
            if (snprintf(abs_path, PATH_MAX, "%s", file_path) < 0) {
                fprintf(stderr, "Unable to format file path '%s', error '%m'.\n", file_path);
                abort();
            }

        } else
            err(EXIT_FAILURE, "Unable to get absolute path of file '%s'.", file_path);
    }

    abs_path[PATH_MAX] = '\0';

    // WARN: Обязательно проверяем что передаваемый basename(3) аргумент не пустой.
    // Потому что на пустой аргумент функция возвращает "."

    if (*abs_path == '\0')
        errx(EXIT_FAILURE, "Error: Empty absolute path, file path '%s'.", file_path);

    char *name = basename(abs_path);

    if (*name == '\0')
        errx(EXIT_FAILURE, "Error: Base name is empty, absolute path '%s'.", abs_path);

    if (snprintf(group, MAX_FILE_PATH, "%s", name) < 0) {
        fprintf(stderr, "Unable to format group name '%s', error '%m'.\n", name);
        abort();
    }
}

void copy_raw_content(const char *const src_dir_path, const char *const dst_dir_path, const char *const file_name)
{
    char src_file_path[MAX_FILE_PATH];
    char dst_file_path[MAX_FILE_PATH];

    format_path(src_file_path, src_dir_path, file_name);

    format_path(dst_file_path, dst_dir_path, file_name);

    LOG_D("Copying file '%s' to '%s'.", src_file_path, dst_file_path);

    const int in_fd = open(src_file_path, O_RDONLY | O_CLOEXEC);

    if (in_fd == -1) {
        LOG_C("Unable to open source file '%s': error '%m'.", src_file_path);
        abort();
    }

    const int out_fd = open(dst_file_path, O_WRONLY | O_CLOEXEC);

    // WARN: При ошибках дескрипторы не закрываем потому что всё равно собираемся падать!

    if (out_fd == -1) {
        LOG_C("Unable to open destination file '%s': error '%m'.", dst_file_path);
        abort();
    }

    // Данных в файле гарантированно менее одного килобайта.

    if (sendfile(out_fd, in_fd, NULL, 1024) == -1) {
        LOG_C("Unable to copy content from file '%s' to '%s': error '%m'.", src_file_path, dst_file_path);
        abort();
    }

    if (close(out_fd) == -1) {
        LOG_C("Unable to close destination file '%s': error '%m'.", dst_file_path);
        abort();
    }

    if (close(in_fd) == -1) {
        LOG_C("Unable to close source file '%s': error '%m'.", src_file_path);
        abort();
    }

    LOG_D("File '%s' has been copied successfully to '%s'.", src_file_path, dst_file_path);
}
