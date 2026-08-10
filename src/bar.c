#include "bar.h"
#include "utils/log.h"
#include "config.h"
#include "ipc.h"

#include <assert.h>
#include <stdlib.h>
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

    ret->backend = init_bar_backend(ret);

    struct bar_ipc *bar_ipc = malloc(sizeof(struct bar_ipc));
    bar_ipc->socket = malloc(sizeof(struct sockaddr_un));

    IPC_socket_init(bar_ipc, SERVER);
    ret->ipc = bar_ipc;
    /* TODO: if the bar resizes, the size of this object will have to change,
     * how should I deal with this? 
     * NOTE: this object is created *after* the configuration event is sent 
     * and therefore the only problematic resize would be by the user at runtime
     */
    ret->pix = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, ret->width, ret->height, NULL, ret->width * PIXMAN_FORMAT_BPP(PIXMAN_a8r8g8b8) / 8);

    return ret;
}

void
bar_destroy(struct bar *bar)
{
    IPC_socket_destroy(bar->ipc, SERVER);
    // TODO: destroy_bar_backend(bar->backend);

    if (bar->displays != NULL)
        free(bar->displays);

    free(bar);
}

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

    while (check_sigint()) {
        if (server_receive_msg(bar->ipc)) {
            if (server_process_msg(bar))
                IPC_send_msg(bar->ipc);
        }

        /* TODO: replace with nanosleep */
        usleep(8000);
    }
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
    return true;
}

bool
bar_refresh_height(struct bar *bar)
{
    if (bar->pos == BAR_LEFT || bar->pos == BAR_RIGHT) {
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

    resize_surfaces(bar);

    bar_refresh_bg_color(bar);
    bar_refresh_border(bar);

    return true;
}

bool
bar_refresh_width(struct bar *bar)
{
    if (bar->pos == BAR_TOP || bar->pos == BAR_BOTTOM) {
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

    resize_surfaces(bar);

    bar_refresh_bg_color(bar);
    bar_refresh_border(bar);

    return true;
}

bool
bar_refresh_position(struct bar *bar)
{
    /* TODO: I really don't like how this function consists of 
     * deferring to another one immediately. I could just directly 
     * call reset_position, but that presents a weird inconsistency 
     * where all but one of my refreshers is in bar.h. What should 
     * I do?
     */
    reset_position(bar);
    return true;
}

bool
bar_refresh_border(struct bar *bar)
{
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
    bar_commit(bar);
    return true;
}
