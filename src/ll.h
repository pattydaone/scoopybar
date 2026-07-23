#ifndef LL_H
#define LL_H

#include "../include/xdg-output-unstable-v1.h"

#include <stdlib.h>
#include "wayland_backend.h"
#include "config.h"

enum type {
	OUTPUT,
	SECTION
};

struct output_node {
	struct output *data;
	struct output_node *next;
};

struct section_node {
	struct section *data;
	struct section_node *next;
};

void LL_push_back(void *head, void *data, enum type type);

#endif
