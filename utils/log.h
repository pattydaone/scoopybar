#ifndef LOG_H
#define LOG_H

#include "src/ipc.h"

#include <stdbool.h>

bool set_log_file(const char *path);

void log_err(const char *file, int line, const char *format, ...);

void log_client_err(struct bar_ipc *ipc, const char *file, int line, const char *format, ...);

void log_dbg(const char *file, int line, int level, const char *format, ...);

void log_info(const char *file, int line, const char *format, ...);

void log_client_info(struct bar_ipc *ipc, const char *file, int line, const char *format, ...);

void log_conf_err(int line, const char *format, ...);

#endif
