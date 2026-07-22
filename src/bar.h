#ifndef BAR_H
#define BAR_H

#include <pixman.h>
#include <stdint.h>

#include "wayland_backend.h"

enum bar_position {
	BAR_TOP,
	BAR_BOTTOM,
	BAR_LEFT,
	BAR_RIGHT
};

struct bar {
	struct bar_backend *backend;

	pixman_color_t background_color;
	float opacity;

	uint32_t height;
	uint32_t width;
	enum bar_position pos;
};

// Will probably take a config struct, but later
struct bar *init_bar();

#endif
