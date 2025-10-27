#ifndef LOG_H
#define LOG_H

#include "ud_log_defines.h"

#include <stdio.h>
#include <stdbool.h>

int log_init();

void log_close(void);

void log_msg(log_level level, const char *fmt, ...);

#endif /* LOG_H */
