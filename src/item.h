#ifndef ITEM_H
#define ITEM_H

#include "bar.h"
#include "utils/config_parser.h"

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
    char *item_name;

    bool draw;

    bool fixed_x;
    bool fixed_y;
    uint32_t x_pos;
    uint32_t y_pos;

    bool fixed_width;
    bool fixed_height;
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

struct bar_item *item_init();

void item_destroy(struct bar_item *item);

bool item_update(struct bar_item *item);

#endif
