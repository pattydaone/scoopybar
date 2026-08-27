#include "misc.h"
#include "log.h"

#include <poll.h>

static void
sync_callback(void *data, struct wl_callback *cb, uint32_t)
{
    bool *done = data;
    *done = true;
    wl_callback_destroy(cb);
}

static const struct wl_callback_listener sync_listener = {.done = &sync_callback};

void /* This feels bad... but wl_display_roundtrip doesn't work... */
my_round_trip(struct wl_display *display)
{
    struct wl_callback *cb = wl_display_sync(display);
    bool done = false;
    wl_callback_add_listener(cb, &sync_listener, &done);

    wl_display_flush(display);
    while (!done) {
        struct pollfd fds[] = {
            {.fd = wl_display_get_fd(display), .events = POLLIN},
        };

        int ret = poll(fds, sizeof(fds) / sizeof(fds[0]), 0);
        if (ret == -1) {
            log_err(__FILE__, __LINE__, "Failed to poll.");
            return;
        }

        if (fds[0].revents & POLLIN) {
            if (wl_display_read_events(display) == -1) {
                log_err(__FILE__, __LINE__, "Failed to read from wayland socket.");
                return;
            }

            while (wl_display_prepare_read(display) != 0) {
                if (wl_display_dispatch_pending(display) == -1) {
                    log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
                    return;
                }
            }

            wl_display_flush(display);
        }
    }
}

volatile sig_atomic_t g_sig;

bool
check_sigint()
{
    if (g_sig == SIGTERM)
        return false;
    if (g_sig == SIGINT)
        return false;
    if (g_sig == SIGABRT)
        return false;

    return true;
}
