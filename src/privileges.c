// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

int drop_privileges(const char *const user_name)
{
    int exit_code = 1;

    assert(user_name != NULL);

    const struct passwd *const pwd = getpwnam(user_name);

    if (pwd == NULL) {
        LOG_E("Unable to get user name '%s', error '%m'.", user_name);
        return 1;
    }

    const long int max_group_count = sysconf(_SC_NGROUPS_MAX);

    if (max_group_count < 1) {
        LOG_E("sysconf(_SC_NGROUPS_MAX): error '%m'.");
        return 1;
    }

    gid_t *const groups = calloc(max_group_count, sizeof(gid_t));

    if (groups == NULL) {
        LOG_E("Unable to allocate memory for groups, error '%m'.");
        abort();
    }

    const gid_t gid = pwd->pw_gid;

    int group_count = max_group_count;

    if (getgrouplist(user_name, gid, groups, &group_count) == -1) {
        LOG_E("Unable to get groups list, error '%m'.");
        goto on_error;
    }

    if (group_count < 1) {
        LOG_E("Error: Invalid group count %d.", group_count);
        goto on_error;
    }

    if (setgroups(group_count, groups) == -1) {
        LOG_E("Unable to set groups, error '%m'.");
        goto on_error;
    }

    if (setresgid(gid, gid, gid) == -1) {
        LOG_E("Unable to set gid, error '%m'.");
        goto on_error;
    }

    const uid_t uid = pwd->pw_uid;

    if (setresuid(uid, uid, uid) == -1) {
        LOG_E("Unable to set uid, error '%m'.");
        goto on_error;
    }

    exit_code = 0;

    LOG_D("Privileges successfully dropped to user '%s'.", user_name);

on_error:

    free(groups);

    return exit_code;
}
