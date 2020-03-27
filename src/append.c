// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cgroup.h"
#include "log.h"
#include "utils.h"

#define PROG_NAME ("cgctl-append")

static void show_usage(void)
{
    // clang-format off
    const char *const usage =
        "Usage: %s [-h|--help] [-d|--debug] GROUP -- PROG [ARGS...]\n"
        "\t-h|--help: show this help;\n"
        "\t-d|--debug: enable debug mode;\n"
        "\tGROUP: group name;\n"
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

    static struct option long_opts[] = {
        { "help", no_argument, 0, 'h' },
        { "debug", no_argument, 0, 'd' },
        { 0, 0, 0, 0 }
    };

    while ((opt = getopt_long(argc, argv, "hd", long_opts, 0)) != -1)
        switch (opt) {
            case 'h':
                show_usage();
                return EXIT_FAILURE;

            case 'd':
                debug = true;
                break;

            default:
                fprintf(stderr, "Error: Unknown argument '%c'.\n", opt);
                return EXIT_FAILURE;
        }

    if (argc - optind < 1) {
        fprintf(stderr, "Error: GROUP is not defined.\n");
        return EXIT_FAILURE;
    }

    if (argc - optind < 2) {
        fprintf(stderr, "Error: PROG is not defined.\n");
        return EXIT_FAILURE;
    }

    const char *const group = argv[optind];

    if (*group == '\0') {
        fprintf(stderr, "Error: Group name is empty.");
        return EXIT_FAILURE;
    }

    char *const prog = argv[optind + 1];

    if (*prog == '\0') {
        fprintf(stderr, "Error: PROG is empty.\n");
        return EXIT_FAILURE;
    }

    log_open(PROG_NAME, debug);

    LOG_D("Started with group='%s', prog='%s'.", group, prog);

    cgroup_append(group);

    LOG_D("Starting program '%s'.", prog);

    log_close(); // WARN: Перед запуском программы закрываем syslog!

    if (execvp(prog, (argv + optind + 1)) == -1) {
        if (errno == ENOENT)
            fprintf(stderr, "Error: Unable to find program '%s'.\n", prog);
        else
            fprintf(stderr, "Error: Unable to start program '%s', error '%m'.\n", prog);
    }

    // WARN: По-идее этот код никогда не должен выполниться.

    return EXIT_FAILURE;
}
