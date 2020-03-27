// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

// максимальный размер файла со списком pid'ов процессов в cgroup
#define MAX_TASKS_FILE_SIZE (10485760)

// название файла с pid'ами процессов в cgroup
static const char *const tasks_name = "tasks";

void kill_all_tasks(const char *const dir_path)
{
    int fd;
    char *lines = NULL;
    char file_path[MAX_FILE_PATH];

    format_path(file_path, dir_path, tasks_name);

    if ((fd = open(file_path, O_RDONLY | O_CLOEXEC)) == -1) {
        LOG_C("Unable to open tasks file '%s', error '%m'.", file_path);
        return;
    }

    if ((lines = malloc(MAX_TASKS_FILE_SIZE + 1)) == NULL) {
        LOG_C("Unable to allocate memory for tasks list, error '%m'.");
        abort();
    }

    /*
     * Сначала читаем весь файл в память, а потом уже выгребаем pid-ы процессов.
     * cgroup это псевдо файловая система, содержимое файла с pid'ами процессов
     * меняется по мере запуска/завершения процессов и поэтому предпочитительно
     * сперва получить полный список, а потом уже его обрабатывать.
     */

    const ssize_t size = read(fd, lines, MAX_TASKS_FILE_SIZE);

    if (size == -1) {
        LOG_E("Unable to read tasks, error '%m'.");
        goto on_error;
    }

    if (size == 0) {
        LOG_D("All tasks are already stopped, nothing to kill.");
        goto on_error;
    }

    /*
     * WARN: прочитав максимально возможное количество байтов, пробуем прочитать ещё,
     * и если это ещё не всё (файл не закончился), то падаем, потому что ситуация
     * критическая - что-то пошло совсем не так и всё очень плохо.
     */

    char tmp;

    if (read(fd, &tmp, 1) == 1) {
        LOG_C("Tasks file is too big.");
        abort();
    }

    lines[size] = '\0';

    for (const char *line = strtok(lines, "\n"); line != NULL; line = strtok(NULL, "\n")) {
        const pid_t pid = str2uint(line);

        if (pid <= 1) {
            LOG_C("Wrong pid value %u ('%s').", pid, line);
            continue;
        }

        LOG_D("Going to kill task with pid %u in '%s'.", pid, file_path);

        // Добиваем процесс сигналом SIGKILL.

        if (kill(pid, SIGKILL) == 0) {
            LOG_D("Task with pid %u has been killed successfully.", pid);
            continue;
        }

        if (errno == ESRCH)
            LOG_D("Pid %u is already exited while sending SIGKILL.", pid);
        else
            LOG_E("Unable to send SIGKILL to pid %u, error '%m'.", pid);
    }

    LOG_D("All found tasks have been killed.");

on_error:

    free(lines);

    if (close(fd) == -1) {
        LOG_E("Unable to close tasks file '%s', error '%m'.", file_path);
        abort();
    }
}

void save_pid2tasks(const char *const dir_path)
{
    const pid_t pid = getpid();

    LOG_D("Adding current pid %u to group '%s'.", pid, dir_path);

    if (write_num(pid, dir_path, tasks_name) != 0) {
        LOG_C("Unable to save pid %u to tasks.", pid);
        abort();
    }
}

bool are_alive_tasks_exist(const char *const dir_path)
{
    char file_path[MAX_FILE_PATH];

    format_path(file_path, dir_path, tasks_name);

    const int fd = open(file_path, O_RDONLY | O_CLOEXEC);

    if (fd == -1) {
        LOG_C("Unable to open tasks file '%s', error '%m'.", file_path);
        abort();
    }

    char tmp;

    const ssize_t size = read(fd, &tmp, 1);

    if (size == -1) {
        LOG_C("Unable to read from tasks file '%s', error '%m'.", file_path);
        abort();
    }

    if (close(fd) == -1) {
        LOG_C("Unable to close tasks file '%s', error '%m'.", file_path);
        abort();
    }

    const bool result = (size == 1);

    LOG_D("%s alive tasks have been found in cgroup '%s'.", ((result) ? "A few" : "No"), dir_path);

    return result;
}
