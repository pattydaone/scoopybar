#ifndef IPC_H
#define IPC_H

#include "bar.h"
#include <stddef.h>

#include <sys/un.h>

struct bar_ipc {
    struct sockaddr_un *socket;
    int socket_fd;
    int accept_fd;

    char msg[1024];
    ssize_t msg_bytes; // For clients sending message
};

enum sock_type {
    SERVER,
    CLIENT
};

bool IPC_socket_init(struct bar_ipc *bar_ipc, enum sock_type type);

void IPC_socket_destroy(struct bar_ipc *bar_ipc, enum sock_type type);

bool server_receive_msg(struct bar_ipc *server);

bool client_receive_msg(struct bar_ipc *server);

bool IPC_send_msg(struct bar_ipc *client);

bool server_process_msg(struct bar *bar);

#endif
