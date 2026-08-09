#ifndef BAR_H
#define BAR_H

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pixman.h>

#include "utils/config_parser.h"

enum bar_position {
    BAR_TOP,
    BAR_BOTTOM,
    BAR_LEFT,
    BAR_RIGHT
};

enum bar_layer {
    BAR_LAYER_BACKGROUND,
    BAR_LAYER_BOTTOM,
    BAR_LAYER_TOP,
    BAR_LAYER_OVERLAY
};

struct bar {
    struct bar_backend *backend;
    struct bar_ipc *ipc;

    pixman_image_t *pix;

    pixman_color_t background_color;
    uint32_t opacity;

    uint32_t height;
    uint32_t width;
    uint32_t height_with_border;
    uint32_t width_with_border;
    enum bar_position pos;
    // TODO: set these two in bar backend
    uint32_t margin;

    struct {
        uint32_t width;

        pixman_color_t color;
    } border;

    char *displays;

    enum bar_layer layer;
};

struct bar *init_bar(struct ConfParser *p);

void bar_destroy(struct bar *bar);

void bar_loop(struct bar *bar);

bool bar_refresh_bg_color(struct bar *bar);

bool bar_refresh_opacity(struct bar *bar);

bool bar_refresh_height(struct bar *bar);

bool bar_refresh_position(struct bar *bar);

bool bar_refresh_border(struct bar *bar);

#endif
