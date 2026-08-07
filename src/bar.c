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
    bar_commit(bar);

    while (check_sigint()) {
        if (server_receive_msg(bar->ipc)) {
            if (server_process_msg(bar))
                IPC_send_msg(bar->ipc);
        }

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
