// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "conf.h"
#include "freezer.h"
#include "log.h"
#include "tasks.h"
#include "utils.h"

// максимальное количество попыток прибить процессы, оставшиеся в cgroup.
#define MAX_ATTEMPTS (10u)

// пауза между попытками, микросекунд
#define ATTEMPTS_DELAY_US (200000u)

/*
 * \fn void apply_limits(const char *const dir_path, const unsigned int cpu_usage, const unsigned int mem_usage)
 * \brief Устанавливает заданные ограничения по CPU и памяти.
 * \param const char *const dir_path: Путь к каталогу /cgroup/$group_name.
 * \param const unsigned int cpu_usage: Ограничение по CPU, в процентах.
 * \param const unsigned int mem_usage: Ограничение по памяти, в процентах.
 */
static void apply_limits(const char *const dir_path, const unsigned int cpu_usage, const unsigned int mem_usage)
{
    struct sysinfo info;

    if (sysinfo(&info) == -1) {
        LOG_C("Unable to get system information, error '%m'.");
        abort();
    }

    LOG_D("System information: RAM %" PRIu64 ", SWAP %" PRIu64 ".", info.totalram, info.totalswap);

    const char *const cpu_limit_name = "cpu.shares";
    const char *const mem_limit_name = "memory.limit_in_bytes";
    const char *const swap_limit_name = "memory.memsw.limit_in_bytes";

    const uint64_t mem_limit = (info.totalram * mem_usage) / 100;

    const uint64_t swap_limit = (info.totalswap * mem_usage) / 100 + mem_limit;

    uint64_t cpu_limit;

    if (read_num(&cpu_limit, CGROUP_ROOT_DIR, cpu_limit_name)) {
        LOG_C("Unable to read CPU limit current value.");
        abort();
    }

    cpu_limit = (cpu_limit * cpu_usage) / 100;

    LOG_D("Setting limits: CPU=%" PRIu64 ", RAM %" PRIu64 ", SWAP %" PRIu64 ".", cpu_limit, mem_limit, swap_limit);

    assert(cpu_limit != 0);
    assert(mem_limit != 0);
    assert(swap_limit != 0);

    // WARN: Лимит со свопом должен быть строго >= лимиту оперативки.
    // Равен он может быть если свопа нет вообще, не считаем это ошибкой!
    assert(swap_limit >= mem_limit);

    if (write_num(cpu_limit, dir_path, cpu_limit_name) != 0) {
        LOG_C("Unable to set CPU limit value %" PRIu64 ".", cpu_limit);
        abort();
    }

    if (write_num(mem_limit, dir_path, mem_limit_name) != 0) {
        LOG_C("Unable to set memory limit %" PRIu64 ".", mem_limit);
        abort();
    }

    if (write_num(swap_limit, dir_path, swap_limit_name) != 0) {
        LOG_C("Unable to set swap limit %" PRIu64 ".", swap_limit);
        abort();
    }
}

void cgroup_append(const char *const name)
{
    char dir_path[MAX_FILE_PATH];

    LOG_D("Adding current process to the existing cgroup '%s'.", name);

    format_path(dir_path, CGROUP_ROOT_DIR, name);

    // Помещаем текущий процесс в существующую cgroup.

    save_pid2tasks(dir_path);
}

void cgroup_create(const char *const name, const unsigned int cpu_usage, const unsigned int mem_usage)
{
    char dir_path[MAX_FILE_PATH];

    format_path(dir_path, CGROUP_ROOT_DIR, name);

    LOG_D("Creating new cgroup '%s' in '%s'.", name, CGROUP_ROOT_DIR);

    if (mkdir(dir_path, 0755) == -1) {
        if (errno != EEXIST) {
            LOG_C("Unable to create directory '%s', error '%m'.", dir_path);
            abort();
        }

        if (are_alive_tasks_exist(dir_path)) {
            LOG_C("Directory '%s' is already exist and a few alive tasks have been found in it.", dir_path);
            abort();
        }

        LOG_E("Directory '%s' is already exist, no alive tasks have been found, reusing the directory.", dir_path);
    }

    // Инициализируем созданный cgroup, копируя в него из корневного каталога
    // содержимое двух файлов - cpuset.cpus и cpuset.mems.

    copy_raw_content(CGROUP_ROOT_DIR, dir_path, "cpuset.cpus");

    copy_raw_content(CGROUP_ROOT_DIR, dir_path, "cpuset.mems");

    // Устанавливаем ограничения.

    apply_limits(dir_path, cpu_usage, mem_usage);

    // Помещаем текущий процесс в только что созданную cgroup.

    save_pid2tasks(dir_path);
}

void cgroup_destroy(const char *const name)
{
    char dir_path[MAX_FILE_PATH];

    format_path(dir_path, CGROUP_ROOT_DIR, name);

    for (size_t i = 1; i <= MAX_ATTEMPTS; i++) {
        /*
         * Нано-оптимизация: прежде чем пускаться во все тяжкие и прибивать процессы,
         * пробуем просто удалить каталог cgroup. Если в нём уже нет ни одного процесса,
         * это получится. Иначе будет ошибка EBUSY.
         */
        if (rmdir(dir_path) == 0) {
            LOG_D("Directory '%s' removed successfully.", dir_path);
            break;
        }

        if (errno == ENOENT) {
            LOG_C("Directory '%s' is already removed.", dir_path);
            break;
        }

        if (errno != EBUSY) {
            LOG_C("Unable to remove directory '%s', error '%m'.", dir_path);
            abort();
        }

        LOG_D("Killing orphaned tasks, attempt %zu of %u.", i, MAX_ATTEMPTS);

        if (freeze_group(dir_path) != 0) {
            LOG_C("Unable to freeze cgroup '%s', leaving tasks running.", name);
            abort();
        }

        kill_all_tasks(dir_path);

        if (unfreeze_group(dir_path) != 0) {
            LOG_C("Unable to unfreeze cgroup '%s', leaving tasks frozen.", name);
            abort();
        }

        /*
         * WARN: боремся с race-condition!
         * Если в cgroup остались какие-то процессы, то rmdir(2) возвращает ошибку EBUSY.
         * Это может случиться если какой-то из процессов не удалось убить потому что он стал зомби
         * или если процесс завис в ожидании возврата syscall.
         * Также было замечено, что даже если ни одного процесса не осталось, то *быстрый*
         * вызов rmdir(2) сразу после пребивания процессов возвращает EBUSY.
         * Почему это происходит - до конца не ясно, поэтому делаем небольшую паузу (200 мс)
         * между попытками удалить cgroup.
         */

        LOG_D("Some tasks are still active, will retry to kill them after %u us sleep.", ATTEMPTS_DELAY_US);

        usleep(ATTEMPTS_DELAY_US);
    }
}
