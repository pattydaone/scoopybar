#include "bar.h"
#include "config.h"
#include "ipc.h"
#include "ll.h"
#include "utils/log.h"

#include <assert.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

#include "wayland_backend.h"

volatile sig_atomic_t g_sig;

struct bar *
init_bar(struct ConfParser *p)
{
    assert(p != NULL);
    struct bar *ret = malloc(sizeof(struct bar));

    enum PARSER_CODES section_code;
    while ((section_code = PARSER_next_section(p)) == SUCCESS) {
        if (!set_opts(ret, p)) {
            return NULL;
        }
    }
    PARSER_clean(p);

    ret->backend = init_bar_backend(ret);
    if (ret->backend == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create bar backend.");
        bar_destroy(ret);
        return NULL;
    }

    struct bar_ipc *bar_ipc = malloc(sizeof(struct bar_ipc));
    bar_ipc->socket = malloc(sizeof(struct sockaddr_un));

    IPC_socket_init(bar_ipc, SERVER);
    ret->ipc = bar_ipc;
    /* TODO: if the bar resizes, the size of this object will have to change,
     * how should I deal with this?
     * NOTE: this object is created *after* the configuration event is sent
     * and therefore the only problematic resize would be by the user at runtime
     */
    ret->pix = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, ret->width, ret->height, NULL,
                                                 ret->width * PIXMAN_FORMAT_BPP(PIXMAN_a8r8g8b8) / 8);

    return ret;
}

void
bar_destroy(struct bar *bar)
{
    log_dbg(__FILE__, __LINE__, 3, "bar destroy called.");
    if (bar->ipc != NULL)
        IPC_socket_destroy(bar->ipc, SERVER);

    if (bar->backend != NULL)
        destroy_bar_backend(bar->backend);

    if (bar->displays != NULL)
        free(bar->displays);

    if (bar->pix != NULL)
        pixman_image_unref(bar->pix);

    free(bar);
}

/* TODO: move this to a utils file or smth. it doesnt need to be here. */
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

void
bar_loop(struct bar *bar)
{
    bar_refresh_bg_color(bar);
    if (bar->border.width > 0)
        bar_refresh_border(bar);
    bar_commit(bar);

    struct bar_backend *backend = bar->backend;

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

            if (!server_process_msg(bar)) {
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

bool
bar_refresh_bg_color(struct bar *bar)
{
    pixman_image_t *fill = pixman_image_create_solid_fill(&bar->background_color);
    if (fill == NULL)
        return false;

    pixman_image_composite(PIXMAN_OP_SRC, fill, NULL, bar->pix, 0, 0, 0, 0, 0, 0, bar->width, bar->height);
    if (!pixman_image_unref(fill))
        return false;

    return true;
}

bool
bar_refresh_opacity(struct bar *bar)
{
    bar->background_color.alpha = bar->opacity;
    if (!bar_refresh_bg_color(bar))
        return false;
    if (!bar_refresh_border(bar))
        return false;
    return true;
}

bool
bar_refresh_height(struct bar *bar)
{
    if (bar->pos == BAR_LEFT || bar->pos == BAR_RIGHT) {
        if (bar->ipc->accept_fd != -1)
            log_client_info(bar->ipc, __FILE__, __LINE__, "Bar position is left or right; doing nothing.");
        return true;
    }

    pixman_image_t *new = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, bar->width, bar->height, NULL,
                                                            bar->width * PIXMAN_FORMAT_BPP(PIXMAN_a8r8g8b8) / 8);
    if (new == NULL)
        return false;

    pixman_image_composite(PIXMAN_OP_SRC, bar->pix, NULL, new, 0, 0, 0, 0, 0, 0, bar->width, bar->height);

    if (!pixman_image_unref(bar->pix))
        return false;

    bar->pix = new;

    resize_buffers(bar);

    bar_refresh_bg_color(bar);
    bar_refresh_border(bar);

    return true;
}

bool
bar_refresh_width(struct bar *bar)
{
    if (bar->pos == BAR_TOP || bar->pos == BAR_BOTTOM) {
        if (bar->ipc->accept_fd != -1)
            log_client_info(bar->ipc, __FILE__, __LINE__, "Bar position is top or bottom; doing nothing.");
        return true;
    }

    pixman_image_t *new = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, bar->width, bar->height, NULL,
                                                            bar->width * PIXMAN_FORMAT_BPP(PIXMAN_a8r8g8b8) / 8);
    if (new == NULL)
        return false;

    pixman_image_composite(PIXMAN_OP_SRC, bar->pix, NULL, new, 0, 0, 0, 0, 0, 0, bar->width, bar->height);

    if (!pixman_image_unref(bar->pix))
        return false;

    bar->pix = new;

    resize_buffers(bar);

    bar_refresh_bg_color(bar);
    bar_refresh_border(bar);

    return true;
}

bool
bar_refresh_border(struct bar *bar)
{
    bar_refresh_bg_color(bar);
    bar->width_with_border = bar->width - 2 * bar->border.width;
    bar->height_with_border = bar->height - 2 * bar->border.width;
    pixman_rectangle16_t rects[4] = {/* Top */
                                     {0, 0, bar->width, bar->border.width},
                                     /* Right */
                                     {bar->width - bar->border.width, 0, bar->border.width, bar->height},
                                     /* Bottom */
                                     {0, bar->height - bar->border.width, bar->width, bar->border.width},
                                     /* Left */
                                     {0, 0, bar->border.width, bar->height}};

    pixman_image_fill_rectangles(PIXMAN_OP_OVER, bar->pix, &bar->border.color, 4, rects);

    return true;
}

bool
bar_refresh_position(struct bar *bar)
{
    struct bar_backend *backend = bar->backend;
    enum zwlr_layer_surface_v1_anchor location;

    switch (bar->pos) {
    case (BAR_TOP):
        location
            = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        break;
    case (BAR_BOTTOM):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        break;
    case (BAR_LEFT):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    case (BAR_RIGHT):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    }

    ll_foreach(backend->outputs, cur)
    {
        struct output *out = cur->data;
        zwlr_layer_surface_v1_set_anchor(out->surface.layer_surface, location);
    }

    return true;
}

bool
bar_refresh_margin(struct bar *bar)
{
    int margin = bar->margin;
    struct bar_backend *backend = bar->backend;

    ll_foreach(backend->outputs, cur)
    {
        struct output *output = cur->data;
        zwlr_layer_surface_v1_set_margin(output->surface.layer_surface, margin, margin, margin, margin);
    }
    /* Trigger configure event to get bar's new size */
    bar_commit(bar);

    /* TODO: think of a better way than this dogshit. */
    wl_display_cancel_read(backend->wl_display);
    wl_display_roundtrip(bar->backend->wl_display);
    while (wl_display_prepare_read(backend->wl_display) != 0) {
        if (wl_display_dispatch_pending(backend->wl_display) == -1) {
            log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
            return false;
        }
    }
    wl_display_flush(backend->wl_display);

    return bar_refresh_height(bar) && bar_refresh_width(bar);
}
