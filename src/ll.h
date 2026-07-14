#ifndef LL_H
#define LL_H

#include "../include/xdg-output-unstable-v1.h"

#include <stdlib.h>
#include "wayland_backend.h"

struct output {
	struct wl_output *output;
	uint32_t width;
	uint32_t height;
	uint32_t scale;
	uint32_t transform;
	char *name;

	struct bar_backend *backend;
};

struct output_node {
	struct output *data;
	struct output_node *next;
};

struct output_node *LL_construct(struct output *output);

void LL_push_back(struct output_node *prev, struct output *output);

// for each ?

#endif
