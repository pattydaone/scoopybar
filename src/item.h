#ifndef ITEM_H
#define ITEM_H

#include "bar.h"

#include <pixman.h>
#include <stdint.h>

enum item_position {
    ITEM_LEFT,
    ITEM_RIGHT,
    ITEM_CENTER,
    ITEM_NOTCH_LEFT,
    ITEM_NOTCH_RIGHT
};

struct bar_item {
    uint32_t x_pos;
    uint32_t y_pos;

    uint32_t width;
    uint32_t height;

    enum item_position pos;

    pixman_image_t *item;

    pixman_color_t background_color;
    uint32_t background_padding_l;
    uint32_t background_padding_r;

    char *icon;
    uint32_t icon_padding_l;
    uint32_t icon_padding_r;

    char *label;
    uint32_t label_padding_l;
    uint32_t label_padding_r;
};

#endif
