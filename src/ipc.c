#include "ipc.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../utils/log.h"

#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

bool client_setup(struct bar_ipc *bar_ipc) {
	struct sockaddr_un *sock = bar_ipc->socket;
	assert( sock != NULL );

	char *sock_path = getenv("SCOOPYBARSOCK");
	if (sock_path == NULL) {
		log_err(__FILE__, __LINE__, "Socket not set. Is scoopybar running?");
		return false;
	}

	strncpy(sock->sun_path, sock_path, 
			offsetof(struct sockaddr_un, sun_path) + strlen(sock->sun_path) + 1);

	if (connect(bar_ipc->socket_fd, (struct sockaddr *)sock, sizeof(*sock)) == -1) {
		log_err(__FILE__, __LINE__, "Failed to connect to socket.");
		return false;
	}

	return true;
}

bool server_setup(struct bar_ipc *bar_ipc) {
	struct sockaddr_un *sock = bar_ipc->socket;
	assert( sock != NULL );

	char *sock_path = "/tmp/scoopybar-socket";
	strncpy(sock->sun_path, sock_path, 
			offsetof(struct sockaddr_un, sun_path) + strlen(sock->sun_path) + 1);
	if (!setenv("SCOOPYBARSOCK", sock_path, 0)) {
		log_err(__FILE__, __LINE__, "Failed to set socket.");
		return false;
	}

	if (bind(bar_ipc->socket_fd, (struct sockaddr *)sock, sizeof(*sock)) == -1) {
		log_err(__FILE__, __LINE__, "Failed to bind to socket.");
		return false;
	}

	// TODO: increase this number; potentially double the amount of widgets ?
	if (listen(bar_ipc->socket_fd, 3) == -1) {
		log_err(__FILE__, __LINE__, "Failed to listen on socket.");
		return false;
	}
	
	return true;

}

bool IPC_socket_init(struct bar_ipc *bar_ipc, enum sock_type type) {
	assert( bar_ipc != NULL );
	bar_ipc->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (bar_ipc->socket_fd == -1) {
		log_err(__FILE__, __LINE__, "Failed to create socket fd.");
		return false;
	}

	if (type == SERVER) {
		return server_setup(bar_ipc);
	}

	return client_setup(bar_ipc);
}

bool IPC_receive_msg(struct bar_ipc *server) {
	size_t b_read = read(server->socket_fd, server->msg, 1024);
	if (b_read == -1) {
		log_err(__FILE__, __LINE__, "Error reading from socket.");
		return false;
	}
	if (b_read == 0) {
		return false; // Nothing sent.
	}
	return true;
}

bool IPC_send_msg(struct bar_ipc *client) {
	size_t b_written = write(client->socket_fd, client->msg, client->msg_bytes);
	if (b_written == -1) {
		log_err(__FILE__, __LINE__, "Failed to write to socket.");
		return false;
	}
	if (b_written != client->msg_bytes) {
		log_info(__FILE__, __LINE__, "Only %zu/%zu bytes sent to socket.", b_written, client->msg_bytes);
		return true;
	}
	return true;
}
