#ifndef IPC_H
#define IPC_H

#include <stdbool.h>
#include <stddef.h>

#include <sys/un.h>

struct bar_ipc {
    struct sockaddr_un *socket;
    int socket_fd;

    char msg[1024];
    size_t msg_bytes; // For clients sending message
};

enum sock_type {
    SERVER,
    CLIENT
};

bool IPC_socket_init(struct bar_ipc *bar_ipc, enum sock_type type);

void IPC_socket_destroy(struct bar_ipc *bar_ipc, enum sock_type type);

bool bar_receive_msg(struct bar_ipc *server);

bool client_send_msg(struct bar_ipc *client);

#endif
