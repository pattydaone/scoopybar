#ifndef BAR_H
#define BAR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

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

	pixman_color_t background_color;
	float opacity;

	uint32_t height;
	uint32_t width;
	enum bar_position pos;
	// TODO: set these two in bar backend
	uint32_t margin;
	uint32_t border_width;
	
	char *displays;

	enum bar_layer layer;
};

struct bar *init_bar(struct ConfParser *p);

#endif
