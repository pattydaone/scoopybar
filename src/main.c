#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>

#include "../utils/config_parser.h"
#include "../utils/log.h"

#include "bar.h"
#include "ipc.h"

extern volatile sig_atomic_t g_sig;

void
signal_handler(int sig)
{
    g_sig = sig;
}

void
print_usage()
{
    printf("scoopybar: usage\n\n");
    printf("Options:\n");

    printf("-c --config=<path>              Path to configuration file.\n"
           "-h --help                       Print this message.\n"
           "-m --message <key>=<value> ...  Send message to exist bar process.\n"
           "-q --query <key>                Query information about the current "
           "bar process.\n");
    printf("\n");
}

bool
prep_and_send_msg(int argc, char **argv)
{
    char msg[1024];
    int msg_index = 0;
    for (int i = 2; i < argc; ++i) {
        char *cur = argv[i];
        int cur_len = strlen(cur);
        memcpy(msg + msg_index, cur, cur_len);
        msg_index += cur_len;

        msg[msg_index] = ' ';
        ++msg_index;
    }
    msg[msg_index - 1] = '\0';

    struct bar_ipc *ipc = malloc(sizeof(struct bar_ipc));
    ipc->socket = malloc(sizeof(struct sockaddr_un));
    if (ipc == NULL || ipc->socket == NULL) {
        log_err(__FILE__, __LINE__, "Failed to allocate ipc structs.");
        IPC_socket_destroy(ipc, CLIENT);
        return false;
    }

    if (!IPC_socket_init(ipc, CLIENT))
        return false;

    ipc->msg_bytes = snprintf(ipc->msg, 1023, "%s", msg);

    if (!client_send_msg(ipc)) {
        log_err(__FILE__, __LINE__, "Failed to send message.");
        IPC_socket_destroy(ipc, CLIENT);
        return false;
    }

    IPC_socket_destroy(ipc, CLIENT);
    return true;
}

int
main(int argc, char **argv)
{
    char config_path[512] = "/home/patrick/Projects/scoopybar/configurations/config.ini";
    static const struct option longoptions[] = {{"message", required_argument, 0, 'm'},
                                                {"query", required_argument, 0, 'q'},
                                                {"config", required_argument, 0, 'c'},
                                                {"help", no_argument, 0, 'h'},
                                                {NULL, no_argument, 0, 0}};

    int opt_char;
    while ((opt_char = getopt_long(argc, argv, "m:q:c:h", longoptions, NULL)) != -1) {
        switch (opt_char) {
        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
        case 'c':
            strncpy(config_path, optarg, 512);
            break;
        case 'm':
            if (!prep_and_send_msg(argc, argv)) {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        case 'q':
            exit(EXIT_SUCCESS);
        }
    }

    const struct sigaction handler = {.sa_handler = &signal_handler};

    sigaction(SIGTERM, &handler, NULL);
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGABRT, &handler, NULL);

    struct ConfParser *p = PARSER_create(config_path, 512);
    if (p == NULL) {
        exit(EXIT_FAILURE);
    }

    struct bar *bar = init_bar(p);
    if (bar == NULL) {
        exit(EXIT_FAILURE);
    }

    bar_loop(bar);

    bar_destroy(bar);

    return EXIT_SUCCESS;
}
