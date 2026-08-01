#ifndef BAR_H
#define BAR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

#include <pixman.h>

#include "../utils/config_parser.h"

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

	pixman_color_t background_color;
	float opacity;

	uint32_t height;
	uint32_t width;
	enum bar_position pos;
	// TODO: set these two in bar backend
	uint32_t margin;

    struct {
        uint32_t l_size;
        uint32_t r_size;
        uint32_t t_size;
        uint32_t b_size;

        pixman_color_t border_color;
    } bar_border;
	
	char *displays;

	enum bar_layer layer;
};

struct bar *init_bar(struct ConfParser *p);

void bar_destroy(struct bar *bar);

void bar_loop(struct bar *bar);

#endif
