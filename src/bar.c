#include "bar.h"
#include "../utils/log.h"
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
    /* NOTE: my calculation of stride here is most certainly incorrect; if the bar needs to resize, this stride 
     * will be invalidated and perhaps not updated. */
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

char *
extract_kv(char *msgs, char *key, char *value)
{
    int i = 0;
    char cur;
    while ((cur = msgs[i]) != '=') {
        if (i > 511) {
            log_err(__FILE__, __LINE__, "Key too long.");
            return NULL;
        }
        if (cur == ' ' || cur == '\0') {
            log_err(__FILE__, __LINE__, "Key without a value.");
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
            log_err(__FILE__, __LINE__, "Value too long.");
            return NULL;
        }
        value[j] = cur;
        ++j;
    }
    value[j] = '\0';
    msgs += j + 1;

    return msgs;
}

bool
find_by_key(struct bar *bar, char *key, char *value)
{
    if (strcmp(key, "bar.background") == 0) {
        bar_set_attribute(bar, value, BAR_BACKGROUND_COLOR);
        return true;
    }
    return true;
}

bool
process_msg(struct bar *bar)
{
    char *msgs = bar->ipc->msg;
    char key[512];
    char value[512];

    while ((msgs = extract_kv(msgs, key, value)) != NULL && msgs[0] != '\0') {
        find_by_key(bar, key, value);
    }
    if (msgs == NULL)
        return false;

    find_by_key(bar, key, value);
    return true;
}

void
bar_loop(struct bar *bar)
{
    bar_commit(bar);
    while (check_sigint()) {
        if (bar_receive_msg(bar->ipc)) {
            process_msg(bar);
            fflush(stdout);
        }

        usleep(8000);
    }
}

bool
bar_refresh_bg_color(struct bar *bar)
{
    pixman_image_t *fill = pixman_image_create_solid_fill(&bar->background_color);
    pixman_image_composite(PIXMAN_OP_SRC, fill, NULL, bar->pix, 0, 0, 0, 0, 0, 0, bar->width, bar->height);
    pixman_image_unref(fill);
    return true;
}
