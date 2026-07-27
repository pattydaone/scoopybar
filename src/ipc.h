#ifndef IPC_H
#define IPC_H

#include <stdbool.h>

#include <sys/socket.h>

struct sockaddr_un IPC_socket_init();

bool set_socket_path(const char *path);

bool open_socket();

#endif
