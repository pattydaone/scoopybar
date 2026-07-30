#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/un.h>
#include <wayland-client.h>

#include "../utils/config_parser.h"
#include "../utils/log.h"

#include "bar.h"
#include "wayland_backend.h"
#include "ipc.h"

void print_usage() {
	printf("Usage: scoopybar\n\n");
	printf("Options:\n");

	printf("-c --config=<path>              Path to configuration file.\n"
		   "-h --help                       Print this message.\n"
		   "-m --message <key>=<value> ...  Send message to exist bar process.\n"
			);
	printf("\n");
}

bool prep_and_send_msg(const char *msg) {
    struct bar_ipc *ipc = malloc(sizeof(struct bar_ipc));
    ipc->socket = malloc(sizeof(struct sockaddr_un));
    if (ipc == NULL || ipc->socket == NULL) {
        log_err(__FILE__, __LINE__, "Failed to allocate ipc structs.");
        IPC_socket_destroy(ipc, CLIENT);
        return false;
    }

    if (!IPC_socket_init(ipc, CLIENT)) return false;

    ipc->msg_bytes = snprintf(ipc->msg, 1023, "%s", msg);

    if (!client_send_msg(ipc)) {
        log_err(__FILE__, __LINE__, "Failed to send message.");
        IPC_socket_destroy(ipc, CLIENT);
        return false;
    }

    IPC_socket_destroy(ipc, CLIENT);
    return true;
}

bool loop(struct bar_ipc *bar_ipc) {
    if (bar_receive_msg(bar_ipc)) {
        printf("%s\n", bar_ipc->msg);
        fflush(stdout);
    }
    if (strcmp(bar_ipc->msg, "END") == 0) return false;

    return true;
}

int main(int argc, char **argv) {
	char config_path[512] = "/home/patrick/Projects/scoopybar/configurations/config.ini";
	static const struct option longoptions[] = {
		{"message", required_argument, 0, 'm'},
		{"config", required_argument, 0, 'c'},
		{"help", no_argument, 0, 'h'},
		{NULL, no_argument, 0, 0}
	};
	
	int opt_char;
	while ((opt_char = getopt_long(argc, argv, "m:c:h", longoptions, NULL)) != -1) {
		switch (opt_char) {
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'c':
				strncpy(config_path, optarg, 512);
				break;
			case 'm':
                if (!prep_and_send_msg(optarg)) {
                    exit(EXIT_FAILURE);
                }
				exit(EXIT_SUCCESS);
		}
	}

	struct ConfParser *p = PARSER_create(config_path, 512);
	if (p == NULL) {
		exit(EXIT_FAILURE);
	}

	struct bar *bar = init_bar(p);
	if (bar == NULL) {
		exit(EXIT_FAILURE);
	}

    struct bar_ipc *bar_ipc = malloc(sizeof(struct bar_ipc));
    bar_ipc->socket = malloc(sizeof(struct sockaddr_un));

    IPC_socket_init(bar_ipc, SERVER) ;

    while (wl_display_dispatch(bar->backend->wl_display)) {
        printf("hi");
        fflush(stdout);
        if (!loop(bar_ipc)) {
            break;
        }
    }

    IPC_socket_init(bar_ipc, SERVER);
	free(bar);

	return EXIT_SUCCESS;
}
