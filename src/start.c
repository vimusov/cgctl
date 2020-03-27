// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cgroup.h"
#include "log.h"
#include "privileges.h"
#include "utils.h"

#define PROG_NAME ("cgctl-start")

static void show_usage(void)
{
    // clang-format off
    const char *const usage =
        "Usage: %s [-h|--help] [-d|--debug] [-g|--group=NAME] [-c|--cpu-usage=NUM] [-m|--mem-usage=NUM] [-u|--user=USER] -- PROG [ARGS...]\n"
        "\t-h|--help: show this help;\n"
        "\t-d|--debug: enable debug mode;\n"
        "\t-g|--group=NAME: group name (same as PROG name by default);\n"
        "\t-c|--cpu-usage=NUM: set maximum CPU usage, percent (100%% by default);\n"
        "\t-m|--mem-usage=NUM: set maximum memory usage, percent (100%% by default);\n"
        "\t-u|--user=USER: drop privileges to USER;\n"
        "\tPROG: program to run;\n"
        "\tARGS: program arguments;\n"
    ;
    // clang-format on

    fprintf(stdout, usage, PROG_NAME);
}

int main(int argc, char **argv)
{
    int opt;
    bool debug = false;
    char *user_name = NULL;
    unsigned int cpu_usage = 100;
    unsigned int mem_usage = 100;
    char group[MAX_FILE_PATH];

    static struct option long_opts[] = {
        { "help", no_argument, 0, 'h' },
        { "debug", no_argument, 0, 'd' },
        { "group", required_argument, 0, 'g' },
        { "cpu-usage", required_argument, 0, 'c' },
        { "mem-usage", required_argument, 0, 'm' },
        { "user", required_argument, 0, 'u' },
        { 0, 0, 0, 0 }
    };

    *group = '\0';

    while ((opt = getopt_long(argc, argv, "hdg:c:m:u:", long_opts, 0)) != -1)
        switch (opt) {
            case 'h':
                show_usage();
                return EXIT_FAILURE;

            case 'd':
                debug = true;
                break;

            case 'g':
                if (*optarg == '\0') {
                    fprintf(stderr, "Error: Group name is empty.\n");
                    return EXIT_FAILURE;
                }
                if (snprintf(group, MAX_FILE_PATH, "%s", optarg) < 0) {
                    fprintf(stderr, "Unable to format string '%s', error '%m'.\n", optarg);
                    abort();
                }
                break;

            case 'c':
                cpu_usage = get_cpu_usage(optarg);
                break;

            case 'm':
                mem_usage = get_mem_usage(optarg);
                break;

            case 'u':
                if (*optarg == '\0') {
                    fprintf(stderr, "Error: User name is empty.\n");
                    return EXIT_FAILURE;
                }
                user_name = optarg;
                break;

            default:
                fprintf(stderr, "Error: Unknown argument '%c'.\n", opt);
                return EXIT_FAILURE;
        }

    if (argc - optind < 1) {
        fprintf(stderr, "Error: PROG is not defined.\n");
        return EXIT_FAILURE;
    }

    char *const prog = argv[optind];

    if (*prog == '\0') {
        fprintf(stderr, "Error: PROG is empty.\n");
        return EXIT_FAILURE;
    }

    // Если название cgroup не задано в опциях, то используем название программы.
    if (*group == '\0')
        get_group_name(prog, group);

    log_open(PROG_NAME, debug);

    LOG_D("Started with group='%s', cpu_usage=%u, mem_usage=%u.", group, cpu_usage, mem_usage);

    cgroup_create(group, cpu_usage, mem_usage);

    if (user_name != NULL) {
        LOG_D("Dropping privileges to user '%s'.", user_name);

        if (drop_privileges(user_name) != 0)
            return EXIT_FAILURE;
    }

    LOG_D("Starting program '%s'.", prog);

    log_close(); // WARN: Перед запуском программы закрываем syslog!

    if (execvp(prog, (argv + optind)) == -1) {
        if (errno == ENOENT)
            fprintf(stderr, "Error: Unable to find program '%s'.\n", prog);
        else
            fprintf(stderr, "Error: Unable to start program '%s', error '%m'.\n", prog);
    }

    // WARN: По-идее этот код никогда не должен выполниться.

    return EXIT_FAILURE;
}
