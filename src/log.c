// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "log.h"

/*
 * \fn void log_open(const char *const prog_name, const bool debug)
 * \brief Открывает syslog и настраивает уровень логирования.
 * \param const char *const prog_name: Название программы.
 * \param const bool debug: Уровень логрования debug включен/выключен.
 */
void log_open(const char *const prog_name, const bool debug)
{
    openlog(prog_name, LOG_PID, LOG_USER);

    const int level = ((debug) ? LOG_DEBUG : LOG_INFO);

    setlogmask(LOG_UPTO(level));
}

/*
 * \fn void log_close(void)
 * \brief Закрывает syslog.
 */
void log_close(void)
{
    closelog();
}
