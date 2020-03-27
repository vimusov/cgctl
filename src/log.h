#ifndef LOG_H_
#define LOG_H_

#include <stdbool.h>
#include <syslog.h>

#define LOG_E(fmt, ...) syslog(LOG_ERR, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_C(fmt, ...) syslog(LOG_CRIT, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#define LOG_D(fmt, ...) syslog(LOG_DEBUG, "%s: " fmt, __FUNCTION__, ##__VA_ARGS__)

void log_open(const char *const prog_name, const bool debug);
void log_close(void);

#endif /* LOG_H_ */
