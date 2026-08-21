#include "ipc.h"
#include "config.h"
#include "utils/log.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

bool
client_setup(struct bar_ipc *bar_ipc)
{
    struct sockaddr_un *sock = bar_ipc->socket;
    assert(sock != NULL);

    // char *sock_path = getenv("SCOOPYBARSOCK");
    char *sock_path = "/tmp/scoopybar-socket";
    if (sock_path == NULL) {
        log_err(__FILE__, __LINE__, "Socket not set. Is scoopybar running?");
        return false;
    }

    strncpy(sock->sun_path, sock_path, sizeof(sock->sun_path) - 1);

    if (connect(bar_ipc->socket_fd, (struct sockaddr *)sock, sizeof(*sock)) == -1) {
        log_err(__FILE__, __LINE__, "Failed to connect to socket.");
        return false;
    }

    return true;
}

bool
server_setup(struct bar_ipc *bar_ipc)
{
    struct sockaddr_un *sock = bar_ipc->socket;
    assert(sock != NULL);

    char *sock_path = "/tmp/scoopybar-socket";
    unlink(sock_path); /* In case a previous instance exited abnormally */
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

bool
IPC_socket_init(struct bar_ipc *bar_ipc, enum sock_type type)
{
    assert(bar_ipc != NULL);

    bar_ipc->accept_fd = -1;
    /* TODO: consider swapping SOCK_STREAM for SOCK_SEQPACKET */
    bar_ipc->socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    bar_ipc->socket->sun_family = AF_UNIX;
    if (bar_ipc->socket_fd == -1) {
        log_err(__FILE__, __LINE__, "Failed to create socket fd.");
        return false;
    }

    if (type == SERVER) {
        return server_setup(bar_ipc);
    }

    return client_setup(bar_ipc);
}

void
IPC_socket_destroy(struct bar_ipc *bar_ipc, enum sock_type type)
{
    assert(bar_ipc != NULL);

    if (close(bar_ipc->socket_fd) == -1)
        log_err(__FILE__, __LINE__, "Failed to close socket fd.");
    if (bar_ipc->accept_fd != -1 && close(bar_ipc->accept_fd) == -1)
        log_err(__FILE__, __LINE__, "Failed to close accept fd.");
    if (bar_ipc->socket != NULL) {
        if (type == SERVER)
            unlink(bar_ipc->socket->sun_path);
        free(bar_ipc->socket);
    }
    free(bar_ipc);

    if (type == SERVER) { /* might not need anymore */
        unsetenv("SCOOPYBARSOCK");
    }
}

bool
server_receive_msg(struct bar_ipc *server)
{
    server->accept_fd = accept(server->socket_fd, NULL, NULL);
    if (server->accept_fd == -1) {
        log_err(__FILE__, __LINE__, "Failed to accept.");
        return false;
    }
    ssize_t b_read = recv(server->accept_fd, server->msg, 1023, 0);
    if (b_read == -1) {
        log_err(__FILE__, __LINE__, "Error reading from socket.");
        return false;
    }
    if (b_read == 0) 
        return false; // Nothing sent.
    server->msg_bytes = b_read;
    server->msg[b_read] = '\0';
    return true;
}

bool
client_receive_msg(struct bar_ipc *client)
{
    ssize_t b_read = recv(client->socket_fd, client->msg, 1023, 0);
    if (b_read == -1) {
        log_err(__FILE__, __LINE__, "Error reading from socket.");
        return false;
    }
    if (b_read == 0)
        return false;

    client->msg_bytes = b_read;
    client->msg[b_read] = '\0';
    return true;
}

bool
IPC_send_msg(struct bar_ipc *client)
{
    ssize_t b_written;
    if (client->accept_fd == -1)
        b_written = send(client->socket_fd, client->msg, client->msg_bytes, 0);
    else 
        b_written = send(client->accept_fd, client->msg, client->msg_bytes, 0);

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

char *
extract_kv(struct bar_ipc *ipc, char *key, char *value)
{
    char *msgs = ipc->msg;
    /* First two chars determine whether this is a message or query */
    msgs += 2;
    int i = 0;
    char cur;
    while ((cur = msgs[i]) != '=') {
        if (i > 511) {
            snprintf(ipc->msg, 1024, "ERROR: Key too long.");
            return NULL;
        }
        if (cur == ' ' || cur == '\0') {
            snprintf(ipc->msg, 1024, "ERROR: Key without a value.");
            return NULL;
        }
        key[i] = cur;
        ++i;
    }
    key[i] = '\0';
    msgs += i + 1;

    int j = 0;
    while ((cur = msgs[j]) != ' ' && cur != '\0') {
        if (j > 511) {
            snprintf(ipc->msg, 1024, "ERROR: Value too long.");
            return NULL;
        }
        value[j] = cur;
        ++j;
    }
    value[j] = '\0';
    msgs += j;
    if (msgs[0] == '\0')
        return msgs;
    else
        return msgs + 1;
}

bool
m_find_by_key(struct bar *bar, char *key, char *value)
{
    /* TODO: Break this down so that before the . and after
     * are treated as separate entities. This won't be very 
     * important until I get to items, I imagine.
     */
    if (strcmp(key, "bar.background") == 0) {
        return bar_set_attribute(bar, value, BAR_BACKGROUND_COLOR);
    }
    else if (strcmp(key, "bar.opacity") == 0) {
        return bar_set_attribute(bar, value, BAR_OPACITY);
    }
    else if (strcmp(key, "bar.height") == 0) {
        return bar_set_attribute(bar, value, BAR_HEIGHT);
    }
    else if (strcmp(key, "bar.width") == 0) {
        return bar_set_attribute(bar, value, BAR_WIDTH);
    }
    else if (strcmp(key, "bar.position") == 0) {
        return bar_set_attribute(bar, value, BAR_POSITION);
    }
    else if (strcmp(key, "bar.margin") == 0) {
        return bar_set_attribute(bar, value, BAR_MARGIN);
    }
    else if (strcmp(key, "bar.border_width") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_WIDTH);
    }
    else if (strcmp(key, "bar.border_color") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_COLOR);
    }
    else if (strcmp(key, "bar.border_opacity") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_OPACITY);
    }
    return true;
}

bool
server_process_msg(struct bar *bar)
{
    char *msgs = bar->ipc->msg;
    char type = msgs[0];
    char key[512];
    char value[512];

    while ((msgs = extract_kv(bar->ipc, key, value)) != NULL && msgs[0] != '\0')
        if (type == 'm' && m_find_by_key(bar, key, value))
            return false;

    if (msgs == NULL)
        return false;

    if (type == 'm' && !m_find_by_key(bar, key, value))
        return false;

    bar->ipc->msg_bytes = snprintf(bar->ipc->msg, 1024, "SUCCESS");

    return true;
}
