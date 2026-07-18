#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

bool set_log_file(const char *path);

void log_err(const char *file, int line, const char *format, ...);

void log_dbg(const char *file, int line, int level, const char *format, ...);

#endif
