#include "bar.h"
#include "config.h"
#include "ipc.h"

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "wayland_backend.h"

volatile sig_atomic_t g_sig;

struct bar *init_bar(struct ConfParser *p) {
	assert( p != NULL );
	struct bar *ret = malloc(sizeof(struct bar));

	enum PARSER_CODES section_code;
	while ((section_code = PARSER_next_section(p)) == SUCCESS) {
		if (!set_opts(ret, p)) {
			return NULL;
		}
	}

	ret->backend = init_bar_backend(ret);

    struct bar_ipc *bar_ipc = malloc(sizeof(struct bar_ipc));
    bar_ipc->socket = malloc(sizeof(struct sockaddr_un));

    IPC_socket_init(bar_ipc, SERVER);
    ret->ipc = bar_ipc;

	return ret;
}

void bar_destroy(struct bar *bar) {
    IPC_socket_destroy(bar->ipc, SERVER);
    // destroy_bar_backend(bar->backend);
    
    if (bar->displays != NULL) free(bar->displays);

    free(bar);
}

bool check_sigint() {
    if (g_sig == SIGTERM) return false;
    if (g_sig == SIGINT) return false;
    if (g_sig == SIGABRT) return false;

    return true;
}

void bar_loop(struct bar *bar) {
    while (check_sigint()) {
        // wl_display_dispatch_pending(bar->backend->wl_display); // This is blocking ?

        if (bar_receive_msg(bar->ipc)) {
            printf("%s\n", bar->ipc->msg);
            fflush(stdout);
        }

        usleep(8000);
    }
}
