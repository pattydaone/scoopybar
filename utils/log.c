
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_LEVEL 3

FILE *log_file = NULL;

bool set_log_file(const char *path) {
	if (!(log_file = fopen(path, "w"))) {
		log_dbg(__FILE__, __LINE__, "Failed to open specified log file; defaulting to stderr\n", 1);
		return false;
	}
	return true;
}

void log_err(const char *file, int line, const char *message) {
	if (log_file == NULL) {
		log_file = stderr;
	}

	fprintf(log_file, "ERROR: %s:%d\t%s\n", file, line, message);
	exit(1);
}

void log_dbg(const char *file, int line, const char *message, int level) {
	if (level > DEBUG_LEVEL) return;

	if (log_file == NULL) {
		log_file = stderr;
	}
	
	fprintf(log_file, "DEBUG: %s:%d\t%s\n", file, line, message);
}
