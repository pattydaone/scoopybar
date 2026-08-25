#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <sys/poll.h>
#include <sys/un.h>
#include <unistd.h>

#include "utils/config_parser.h"
#include "utils/log.h"

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
           "-m --message <key>=<value> ...  Send message to existing bar process.\n"
           "-q --query <key>                Query information about the current "
           "bar process.\n");
    printf("\n");
}

bool
prep_and_send_msg(struct bar_ipc *ipc, char type, int argc, char **argv)
{
    char msg[1024];
    msg[0] = type;
    msg[1] = ' ';
    int msg_index = 2;
    for (int i = 2; i < argc; ++i) {
        char *cur = argv[i];
        int cur_len = strlen(cur);
        memcpy(msg + msg_index, cur, cur_len);
        msg_index += cur_len;

        msg[msg_index] = ' ';
        ++msg_index;
    }
    msg[msg_index - 1] = '\0';

    ipc->msg_bytes = snprintf(ipc->msg, 1023, "%s", msg);

    if (!IPC_send_msg(ipc)) {
        log_err(__FILE__, __LINE__, "Failed to send message.");
        IPC_socket_destroy(ipc, CLIENT);
        return false;
    }

    return true;
}

bool
run_client(char type, int argc, char **argv)
{
    struct bar_ipc *ipc = malloc(sizeof(struct bar_ipc));
    ipc->socket = malloc(sizeof(struct sockaddr_un));
    if (ipc == NULL || ipc->socket == NULL) {
        log_err(__FILE__, __LINE__, "Failed to allocate ipc structs.");
        goto out;
    }

    if (!IPC_socket_init(ipc, CLIENT))
        goto out;

    if (!prep_and_send_msg(ipc, type, argc, argv))
        goto out;

    int s;
    struct pollfd fd[] = {{ .fd = ipc->socket_fd, .events = POLLIN }};
    for (s = 0; s < 20; ++s) {
        /* Blocks until it receives SUCCESS message... scary...
         * but it seems if I don't do this sometimes I'll reach 
         * the timeout before the bar is able to respond, which 
         * causes the bar to crash as well....
         */
        if (poll(fd, sizeof(fd)/sizeof(fd[0]), -1) == -1) {
            log_err(__FILE__, __LINE__, "Failed to poll.");
            goto out;
        }
        
        if (fd[0].revents & POLLIN) {
            if (!client_receive_msg(ipc)) 
                goto out;
            if (strcmp(ipc->msg, "SUCCESS") == 0) {
                IPC_socket_destroy(ipc, CLIENT);
                return true;
            }
            fprintf(stderr, "%s\n", ipc->msg);
        }
    }

    if (s >= 20) {
        log_err(__FILE__, __LINE__, "Bar didn't return message.");
        goto out;
    }

out:
    IPC_socket_destroy(ipc, CLIENT);
    return false;
}

int
main(int argc, char **argv)
{
    const char *home_dir = getenv("HOME");
    const char *rest = "/.config/scoopybar/config.ini";
    char config_path[strlen(home_dir) + strlen(rest) + 1];
    char *n = stpcpy(config_path, home_dir);
    strcpy(n, rest);

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
            if (!run_client(opt_char, argc, argv)) {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        case 'q':
            if (!run_client(opt_char, argc, argv)) {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        case ':':
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }

    const struct sigaction handler = {.sa_handler = &signal_handler};

    sigaction(SIGTERM, &handler, NULL);
    sigaction(SIGINT, &handler, NULL);
    sigaction(SIGABRT, &handler, NULL);

    struct ConfParser *p = PARSER_create(config_path, 512);
    if (p == NULL) {
        log_err(__FILE__, __LINE__, "%s: path not found", config_path);
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
