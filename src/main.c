// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "conf.h"
#include "cgroup.h"
#include "log.h"
#include "utils.h"

#define PROG_NAME ("cgctl")

typedef struct
{
    bool debug; // режим отладки включен/выключен
    unsigned int cpu_usage; // ограничение по CPU, в процентах
    unsigned int mem_usage; // ограничение по памяти, в процентах
    char *group; // название cgroup
} options_t;

static void show_usage(void)
{
    // clang-format off
    const char *const usage =
        "Usage: %s [--help] [--options=OPTIONS] SCRIPT ACTION\n"
        "\t--help: show this help;\n"
        "\t--options=OPTIONS: set custom options;\n"
        "\tOPTIONS: debug,group=NAME,cpu_usage=NUM,mem_usage=NUM\n"
        "\t\tdebug: enable debug mode;\n"
        "\t\tgroup=NAME: use group name (same as script by default);\n"
        "\t\tcpu_usage=NUM: set maximum CPU usage, percent (100%% by default);\n"
        "\t\tmem_usage=NUM: set maximum memory usage, percent (100%% by default);\n"
        "\tSCRIPT: initscript to run;\n"
        "\tACTION: initscript action (start|stop|restart|etc);\n"
        "WARNING! DO NOT PUT space between '--options' and OPTIONS, use '=' only!!!\n"
    ;
    // clang-format on

    fprintf(stdout, usage, PROG_NAME);
}

/*
 * \fn int parse_options(char *opts_value, options_t *opts)
 * \brief Парсит и заполняет опции для cgroup.
 * \param char *opts_value: Строка с опциями.
 * \param options_t *opts: Опции.
 * \return 1 в случае ошибки; 0 если опции распарсены успешно.
 */
static int parse_options(char *opts_value, options_t *opts)
{
    /*
     * WARN: особая обработка опций командной строки.
     *
     * Причина появления данной функции в том что обработка опций у shebang
     * вообще никак, никем и никогда не была стандартизована.
     *
     *     https://stackoverflow.com/a/4304187
     *
     * Например, если задать такое:
     *
     *     #!/usr/bin/cgctl --debug --cpu-count=50 --mem-usage=25
     *
     * То в argv[1] окажется не '--debug' как можно было бы ожидать,
     * а полностью вся строка '--debug --cpu-count=50 --mem-usage=25'.
     *
     * Функция getopt(3) не готова к такому повороту и не может нормально
     * распарсить опции. Поэтому приходится выкручиваться и делать одну
     * опцию '--options=OPTIONS', причем использование пробела вместо '='
     * в ней также недопустимо, getopt(3) не может такое распарсить.
     *
     * Получив опции в виде 'debug,cpu_usage=50', уже можно использовать
     * стандартную функцию getsubopt(3) чтобы не писать велосипеды,
     * разбирая аргументы командной строки самостоятельно.
     */

    enum
    {
        DEBUG_OPT = 0,
        GROUP_OPT,
        CPU_USAGE_OPT,
        MEM_USAGE_OPT
    };

    // clang-format off
    char *const names[] = {
        [DEBUG_OPT] = "debug",
        [GROUP_OPT] = "group",
        [CPU_USAGE_OPT] = "cpu_usage",
        [MEM_USAGE_OPT] = "mem_usage",
        NULL
    };
    // clang-format on

    // Сохраняем оригинальный указатель чтобы при необходимости показать ошибку,
    // т.к. opts_value меняется в процессе разбора опций.
    char *org_opts_value = opts_value;

    while (*opts_value != '\0') {
        char *value = NULL;

        const int opt = getsubopt(&opts_value, names, &value);

        if (opt == -1) {
            fprintf(stderr, "Error: Invalid options value '%s'.\n", org_opts_value);
            return 1;
        }

        switch (opt) {
            case DEBUG_OPT:
                opts->debug = true;
                continue;

            case GROUP_OPT:
                if (value != NULL) {
                    opts->group = value;
                    continue;
                }
                break;

            case CPU_USAGE_OPT:
                if (value != NULL) {
                    opts->cpu_usage = get_cpu_usage(value);
                    continue;
                }
                break;

            case MEM_USAGE_OPT:
                if (value != NULL) {
                    opts->mem_usage = get_mem_usage(value);
                    continue;
                }
                break;

            default:
                fprintf(stderr, "Error: Unknown option '%s'.\n", ((value == NULL) ? "?" : value));
                return 1;
        }

        fprintf(stderr, "Error: Missing value for option '%s'.\n", names[opt]);
        return 1;
    }

    return 0;
}

/*
 * \fn int run_process(char **argv)
 * \brief Запускает init-скрипт в bash и дожидается его завершения.
 * \param char *const script: init-скрипт.
 * \param char *const action: Аргумент скрипта, действие (start|stop|etc).
 * \return Код выхода запущенного init-скрипта.
 */
static int run_process(char *const script, char *const action)
{
    LOG_D("Exec init-script '%s' with action '%s'.", script, action);

    const pid_t child_pid = fork();

    if (child_pid == -1) {
        LOG_C("Unable to fork() process, error '%m'.");
        abort();
    }

    if (child_pid == 0) {
        if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1) {
            LOG_C("Unable to set PR_SET_PDEATHSIG, error '%m'.");
            abort();
        }

        log_close(); // WARN: Обязательно закрываем syslog в дочернем процессе!

        char *argv[] = { SHELL, script, action, NULL };

        if (execv(argv[0], argv) == -1) {
            if (errno == ENOENT)
                fprintf(stderr, "Error: Unable to find init-script '%s'.\n", script);
            else
                fprintf(stderr, "Unable to run init-script '%s' with action '%s', error '%m'.\n", script, action);
        }

        abort(); // WARN: По-идее этот код никогда не должен выполниться.
    }

    int status = 1;

    if (waitpid(child_pid, &status, 0) == -1) {
        LOG_C("Unable to wait for pid %u, error '%m'.", child_pid);
        abort();
    }

    const int exit_code = WEXITSTATUS(status);

    LOG_D("Process %u ('%s' '%s') exit code %d.", child_pid, script, action, exit_code);

    return exit_code;
}

int main(int argc, char **argv)
{
    int opt;
    char group[MAX_FILE_PATH];

    // Значения по-умолчанию.
    options_t opts = {
        .debug = false,
        .cpu_usage = 100,
        .mem_usage = 100,
        .group = NULL
    };

    static struct option long_opts[] = {
        { "help", no_argument, 0, 'h' },
        { "options", required_argument, 0, 'o' },
        { 0, 0, 0, 0 }
    };

    *group = '\0';

    while ((opt = getopt_long(argc, argv, "ho:", long_opts, 0)) != -1)
        switch (opt) {
            case 'h':
                show_usage();
                return EXIT_FAILURE;

            case 'o':
                if (parse_options(optarg, &opts) == 0)
                    break;
                else
                    return EXIT_FAILURE;

            default:
                fprintf(stderr, "Error: Unknown argument '%c'.\n", opt);
                return EXIT_FAILURE;
        }

    if (argc - optind != 2) {
        fprintf(stderr, "Error: Options SCRIPT and ACTION are not defined.\n");
        return EXIT_FAILURE;
    }

    char *const script = argv[optind];

    if (*script == '\0') {
        fprintf(stderr, "Error: SCRIPT is empty.\n");
        return EXIT_FAILURE;
    }

    char *const action = argv[optind + 1];

    if (*action == '\0') {
        fprintf(stderr, "Error: ACTION is empty.\n");
        return EXIT_FAILURE;
    }

    const char *const opt_group = opts.group;

    if (opt_group != NULL && *opt_group == '\0') {
        fprintf(stderr, "Error: Group name is empty.\n");
        return EXIT_FAILURE;
    }

    // Если название cgroup не задано в опциях, то используем название init-скрипта.
    if (opt_group == NULL)
        get_group_name(script, group);

    else {
        if (snprintf(group, MAX_FILE_PATH, "%s", opt_group) < 0) {
            fprintf(stderr, "Unable to format string '%s', error '%m'.\n", opt_group);
            abort();
        }
    }

    const unsigned int cpu_usage = opts.cpu_usage;
    const unsigned int mem_usage = opts.mem_usage;

    log_open(PROG_NAME, opts.debug);

    LOG_D("Started with script='%s', action='%s' in group='%s', cpu_usage=%u, mem_usage=%u.",
        script, action, group, cpu_usage, mem_usage);

    int exit_code = 1;

    if (strcmp(action, "start") == 0) {
        // По команде на запуск создаём cgroup, затем запускаем init-скрипт.
        cgroup_create(group, cpu_usage, mem_usage);
        exit_code = run_process(script, action);

    } else if (strcmp(action, "stop") == 0) {
        // По команде на остановку останавливаем init-скрипт, затем удаляем cgroup.
        exit_code = run_process(script, action);
        cgroup_destroy(group);

    } else if (strcmp(action, "restart") == 0) {
        // WARN: По команде на перезапуск выполняем сначала stop, затем start;
        // вызывать init-скрипт c restart нельзя т.к. в нём restart может быть
        // реализован очень странными методами.

        run_process(script, "stop"); // WARN: Игнорируем код выхода!
        cgroup_destroy(group);

        cgroup_create(group, cpu_usage, mem_usage);
        exit_code = run_process(script, "start");

    } else {
        // По любой другой команде просто выполняем её.
        exit_code = run_process(script, action);
    }

    LOG_D("Script '%s' with action '%s' exited with code %d.", script, action, exit_code);

    log_close();

    return exit_code;
}
