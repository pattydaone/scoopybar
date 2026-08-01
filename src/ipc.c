#include "ipc.h"
#include "../utils/log.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

bool client_setup(struct bar_ipc *bar_ipc) {
	struct sockaddr_un *sock = bar_ipc->socket;
	assert( sock != NULL );

	// char *sock_path = getenv("SCOOPYBARSOCK");
    char *sock_path = "/tmp/scoopybar-socket";
	if (sock_path == NULL) {
		log_err(__FILE__, __LINE__, "Socket not set. Is scoopybar running?");
		return false;
	}

	strncpy(sock->sun_path, sock_path, sizeof(sock->sun_path) - 1);

	if (connect(bar_ipc->socket_fd, (struct sockaddr *)sock, sizeof(*sock)) == -1) {
		log_err(__FILE__, __LINE__, "Failed to connect to socket.");
        /*
        switch (errno) {
            case EACCES:
                printf("eaccess");
                break;
            case EPERM:
                printf("eperm");
                break;
            case EADDRNOTAVAIL:
                printf("addrnotavail");
                break;
            case EADDRINUSE:
                printf("Address in use");
                break;
            case EAFNOSUPPORT:
                printf("eafnosupport");
                break;
            case EAGAIN:
                printf("eagain");
                break;
            case EALREADY:
                printf("ealready");
                break;
            case EBADF:
                printf("ebadf");
                break;
            case ECONNREFUSED:
                printf("connection refused");
                break;
            case EFAULT:
                printf("efault");
                break;
            case EINPROGRESS:
                printf("in progress");
                break;
            case EINTR:
                printf("eintr");
                break;
            case ENOTSOCK:
                printf("notsock");
                break;
            case EPROTOTYPE:
                printf("prototype");
                break;
            case ETIMEDOUT:
                printf("timdout");
                break;
            default:
                printf("idk"); // keeps outputting this. really glad i made a switch case for all the other fucking ones
                break;
        }
        printf("\n"); */
		return false;
	}

	return true;
}

bool server_setup(struct bar_ipc *bar_ipc) {
	struct sockaddr_un *sock = bar_ipc->socket;
	assert( sock != NULL );

	char *sock_path = "/tmp/scoopybar-socket";
	strncpy(sock->sun_path, sock_path, sizeof(sock->sun_path) - 1);
	if (setenv("SCOOPYBARSOCK", sock_path, 1) == -1) {
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
    bar_ipc->socket->sun_family = AF_UNIX;
	if (bar_ipc->socket_fd == -1) {
		log_err(__FILE__, __LINE__, "Failed to create socket fd.");
		return false;
	}
    if (fcntl(bar_ipc->socket_fd, F_SETFD, FD_CLOEXEC) == -1) {
        log_err(__FILE__, __LINE__, "Failed to set FD_CLOEXEC.");
        return false;
    }
    if (fcntl(bar_ipc->socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        log_err(__FILE__, __LINE__, "Failed to set nonblocking on fd.");
    }

	if (type == SERVER) {
		return server_setup(bar_ipc);
	}

	return client_setup(bar_ipc);
}

void IPC_socket_destroy(struct bar_ipc *bar_ipc, enum sock_type type) {
    assert( bar_ipc != NULL );

    if (close(bar_ipc->socket_fd) == -1) log_err(__FILE__, __LINE__, "Failed to close socket fd.");
    if (bar_ipc->socket != NULL) {
        if (type == SERVER) unlink(bar_ipc->socket->sun_path);
        free(bar_ipc->socket);
    }
    free(bar_ipc);

    if (type == SERVER) { // might not need anymore
        unsetenv("SCOOPYBARSOCK");
    }
}

bool bar_receive_msg(struct bar_ipc *server) {
    int accept_fd = accept(server->socket_fd, NULL, NULL);
    if (accept_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { /* No connection present. */
            errno = 0;
            return false;
        }
        log_err(__FILE__, __LINE__, "Failed to accept.");
        return false;
    }
	size_t b_read = read(accept_fd, server->msg, sizeof(server->msg));
	if (b_read == -1) {
		log_err(__FILE__, __LINE__, "Error reading from socket.");
        close(accept_fd);
		return false;
	}
	if (b_read == 0) {
        close(accept_fd);
		return false; // Nothing sent.
	}
    server->msg[b_read] = '\0';
    close(accept_fd);
	return true;
}

bool client_send_msg(struct bar_ipc *client) {
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
