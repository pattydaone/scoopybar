#include "ll.h"
#include "wayland_backend.h"

void push_back_output(struct output_node *prev, struct output *data) {
	struct output_node *node = malloc(sizeof(struct output_node));

	node->data = data;
	node->next = NULL;

	if (prev == NULL) {
		prev = node;
	}
	else {
		prev->next = node;
	}
}

void push_back_surface(struct surface_node *prev, struct surface *data) {
	struct surface_node *node = malloc(sizeof(struct surface_node));

	node->data = data;
	node->next = NULL;

	if (prev == NULL) {
		prev = node;
	}
	else {
		prev->next = node;
	}
}

void LL_push_back(void *prev, void *data, enum type type) {
	switch (type) {
		case OUTPUT:
			push_back_output(prev, data);
			break;
		case SURFACE:
			push_back_surface(prev, data);
			break;
	}
}
