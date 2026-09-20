#include "bar_manager.h"

#include "message_handler.h"
#include "utils/log.h"
#include "utils/misc.h"

#include <stdlib.h>
#include <sys/poll.h>

struct bar_manager *
init_bar_manager(struct ConfParser *p)
{
    struct bar_manager *ret = malloc(sizeof(struct bar_manager));

    ret->bar = init_bar(p);
    if (ret->bar == nullptr) {
        log_err(__FILE__, __LINE__, "Failed to create bar.");
        return nullptr;
    }

    ret->backend = ret->bar->backend;
    ret->queue = init_queue();

    return ret;
}

void
destroy_bar_manager(struct bar_manager *manager)
{
    bar_destroy(manager->bar);
    free(manager);
}

void
event_loop(struct bar_manager *manager)
{
    struct bar *bar = manager->bar;
    struct bar_backend *backend = manager->backend;

    bar_refresh_bg_color(bar);
    if (bar->border.width > 0)
        bar_refresh_border(bar);
    bar_commit(bar);

    while (wl_display_prepare_read(backend->wl_display) != 0) {
        if (wl_display_dispatch_pending(backend->wl_display) == -1) {
            log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
            return;
        }
    }

    wl_display_flush(backend->wl_display);

    struct pollfd fds[] = {{.fd = bar->ipc->socket_fd, .events = POLLIN},
                           {.fd = wl_display_get_fd(backend->wl_display), .events = POLLIN}};
    while (check_sigint()) {
        int ret = poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
        if (ret == -1) {
            log_err(__FILE__, __LINE__, "Failed to poll.");
            return;
        }

        if (fds[0].revents & POLLIN) {
            if (!server_receive_msg(bar->ipc)) {
                log_err(__FILE__, __LINE__, "Failed to receive message.");
                return;
            }

            if (!process_msg(bar)) {
                log_err(__FILE__, __LINE__, "Failed to process message.");
                return;
            }

            if (!IPC_send_msg(bar->ipc)) {
                log_err(__FILE__, __LINE__, "Failed to reply to message.");
                return;
            }
        }

        if (fds[1].revents & POLLIN) {
            if (wl_display_read_events(backend->wl_display) == -1) {
                log_err(__FILE__, __LINE__, "Failed to read from wayland socket.");
                goto err;
            }

            while (wl_display_prepare_read(backend->wl_display) != 0) {
                if (wl_display_dispatch_pending(backend->wl_display) == -1) {
                    log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
                    goto err;
                }
            }

            wl_display_flush(backend->wl_display);
        }
    }
err:
    wl_display_cancel_read(backend->wl_display);
}
