#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_LEVEL 3

FILE *log_file = NULL;

bool
set_log_file(const char *path)
{
    if (!(log_file = fopen(path, "w"))) {
        log_dbg(__FILE__, __LINE__, 1, "Failed to open specified log file; defaulting to stderr");
        return false;
    }
    return true;
}

void
log_err(const char *file, int line, const char *format, ...)
{
    if (log_file == NULL) {
        log_file = stderr;
    }

    fprintf(log_file, "ERROR: %s:%d\t", file, line);

    va_list va;
    va_start(va, format);
    vfprintf(log_file, format, va);
    va_end(va);

    fputs("\n", log_file);
}

void
log_client_err(struct bar_ipc *ipc, const char *file, int line, const char *format, ...)
{
    if (ipc->accept_fd == -1) {
        log_err(__FILE__, __LINE__, "No active connection to send errors to.");
        return;
    }

    int n = snprintf(ipc->msg, 1024, "ERROR: %s:%d\t", file, line);

    va_list va;
    va_start(va, format);
    n += vsnprintf(ipc->msg + n, 1024 - n, format, va);
    va_end(va);

    ipc->msg_bytes = n + 1;
    IPC_send_msg(ipc);
}

void
log_dbg(const char *file, int line, int level, const char *format, ...)
{
    if (level > DEBUG_LEVEL)
        return;
    if (log_file == NULL) {
        log_file = stderr;
    }
    fprintf(log_file, "DEBUG: %s:%d\t", file, line);

    va_list va;
    va_start(va, format);
    vfprintf(log_file, format, va);
    va_end(va);

    fputs("\n", log_file);
}

void
log_info(const char *file, int line, const char *format, ...)
{
    if (log_file == NULL) {
        log_file = stderr;
    }
    fprintf(log_file, "INFO: %s:%d\t", file, line);

    va_list va;
    va_start(va, format);
    vfprintf(log_file, format, va);
    va_end(va);

    fputc('\n', log_file);
}

void
log_client_info(struct bar_ipc *ipc, const char *file, int line, const char *format, ...)
{
    if (ipc->accept_fd == -1) {
        log_err(__FILE__, __LINE__, "No active connection to send info to.");
        return;
    }

    int n = snprintf(ipc->msg, 1024, "INFO: %s:%d\t", file, line);

    va_list va;
    va_start(va, format);
    n += vsnprintf(ipc->msg + n, 1024 - n, format, va);
    va_end(va);

    ipc->msg_bytes = n + 1;
    IPC_send_msg(ipc);
}

void
log_conf_err(int line, const char *format, ...)
{
    if (log_file == NULL) {
        log_file = stderr;
    }

    fprintf(log_file, "Configuration error: line %d\t", line);

    va_list va;
    va_start(va, format);
    vfprintf(log_file, format, va);
    va_end(va);

    fputc('\n', log_file);
}
