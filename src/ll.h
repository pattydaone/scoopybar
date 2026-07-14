#ifndef LL_H
#define LL_H

#include "../include/xdg-output-unstable-v1.h"

#include <stdlib.h>
#include "wayland_backend.h"

enum type {
	OUTPUT,
	SURFACE
};

struct output_node {
	struct output *data;
	struct output_node *next;
};

struct surface_node {
	struct surface *data;
	struct surface_node *next;
};

void LL_push_back(void *prev, void *data, enum type type);

// for each ?

#endif
