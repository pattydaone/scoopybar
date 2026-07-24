#ifndef LL_H
#define LL_H

#include "../include/xdg-output-unstable-v1.h"

#include "wayland_backend.h"

enum type {
	OUTPUT,
};

struct output_node {
	struct output *data;
	struct output_node *next;
};

void LL_push_back(void *head, void *data, enum type type);

#endif
