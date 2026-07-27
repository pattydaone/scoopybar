#include "ipc.h"

#include <stdlib.h>

#include "../utils/log.h"

bool set_socket_path(const char *path) {
	if (!setenv("SCOOPYBARSOCK", path, 0)) {
		log_err(__FILE__, __LINE__, "Failed to set socket environment variable.");
		return false;
	}
	return true;
}

bool open_socket() {
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		log_err(__FILE__, __LINE__, "Failed to open socket.");
		return false;
	}

	return true;
}
