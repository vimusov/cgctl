// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"

// максимальное количество попыток сколько ждать применения заморозки/разморозки
#define MAX_ATTEMPTS (10u)

// пауза между попытками, микросекунд
#define ATTEMPTS_DELAY_US (200000u)

static int _freeze_group(const char *const dir_path, const bool do_freeze)
{
    static const char *const frozen_state = "FROZEN";
    static const char *const thawed_state = "THAWED";

    char *eol;
    char buf[16];
    int exit_code = 1;
    char file_path[MAX_FILE_PATH];
    const char *target_state; // состояние, к которому нужно прийти
    const char *current_state; // состояние, в котором cgroup находится сейчас

    if (do_freeze) {
        target_state = frozen_state;
        current_state = thawed_state;
        LOG_D("Going to freeze group '%s', changing state: %s => %s.", dir_path, current_state, target_state);
    } else {
        target_state = thawed_state;
        current_state = frozen_state;
        LOG_D("Going to unfreeze group '%s', changing state: %s => %s.", dir_path, current_state, target_state);
    }

    format_path(file_path, dir_path, "freezer.state");

    FILE *const fp = fopen(file_path, "r+e");

    if (fp == NULL) {
        LOG_C("Unable to open freezer file '%s', error '%m'.", file_path);
        abort();
    }

    setbuf(fp, NULL);

    if (fgets(buf, sizeof(buf), fp) == NULL) {
        LOG_C("Unable to read line from '%s', error '%m'.", file_path);
        abort();
    }

    if ((eol = strchr(buf, '\n')) != NULL)
        *eol = '\0'; // убираем перевод строки

    LOG_D("Got current state '%s' from '%s'.", buf, file_path);

    // WARN: текущее состояние не совпадает с ожидаемым - панико!
    if (strcmp(buf, current_state) != 0) {
        LOG_C("Unexpected current state: expected '%s' but '%s' found.", current_state, buf);
        abort();
    }

    LOG_D("Writing state %s to file '%s'.", target_state, file_path);

    if (fseek(fp, 0, SEEK_SET) != 0) {
        LOG_C("Unable to move file '%s' to the beginning, error '%m'.", file_path);
        abort();
    }

    if (fprintf(fp, "%s\n", target_state) != (int) (strlen(target_state) + 1)) { // +1 на перевод строки
        LOG_C("Unable to write target state %s to file '%s', error '%m'.", target_state, file_path);
        abort();
    }

    // WARN: После смены состояния путём записи в файл дожидаемся пока оно реально сменится.
    // Это может занять некоторое время.

    for (size_t i = 1; i <= MAX_ATTEMPTS; i++) {
        LOG_D("Waiting for state change from %s to %s, attempt %zu of %u.", current_state, target_state, i, MAX_ATTEMPTS);

        if (fseek(fp, 0, SEEK_SET) != 0) {
            LOG_C("Unable to move file '%s' to the beginning, error '%m'.", file_path);
            abort();
        }

        if (fgets(buf, sizeof(buf), fp) == NULL) {
            LOG_C("Unable to read line from '%s', error '%m'.", file_path);
            abort();
        }

        if ((eol = strchr(buf, '\n')) != NULL)
            *eol = '\0'; // убираем перевод строки

        LOG_D("Got current state '%s' from '%s'.", buf, file_path);

        if (strcmp(buf, target_state) == 0) {
            LOG_D("Group '%s' has been %s successfully.", dir_path, ((do_freeze) ? "frozen" : "unfrozen"));
            exit_code = 0;
            break;
        }

        LOG_D("Intermediate state '%s', will retry after pause %u us.", buf, ATTEMPTS_DELAY_US);

        usleep(ATTEMPTS_DELAY_US);
    }

    if (fclose(fp) == EOF) {
        LOG_C("Unable to close file '%s', error '%m'.", file_path);
        abort();
    }

    return exit_code;
}

int freeze_group(const char *const dir_path)
{
    return _freeze_group(dir_path, true);
}

int unfreeze_group(const char *const dir_path)
{
    return _freeze_group(dir_path, false);
}
