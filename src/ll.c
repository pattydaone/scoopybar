#include "ll.h"
#include "wayland_backend.h"

#include <stdlib.h>

void push_back_output(struct output_node *head, struct output *data) {
	struct output_node *node = malloc(sizeof(struct output_node));

	node->data = data;
	node->next = NULL;

	if (head == NULL) {
		head = node;
		return;
	}
	while (head->next != NULL) {
		head = head->next;
	}

	head->next = node;
}

void LL_push_back(void *head, void *data, enum type type) {
	switch (type) {
		case OUTPUT:
			push_back_output(head, data);
			break;
	}
}
