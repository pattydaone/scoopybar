#include "ll.h"

struct output_node *LL_construct(struct output *output) {
	struct output_node *ret = malloc(sizeof(struct output_node));
	ret->data = output;
	ret->next = NULL;
	return ret;
}

void LL_push_back(struct output_node *prev, struct output *output) {
	struct output_node *node = malloc(sizeof(struct output_node));
	node->data = output;
	node->next = NULL;

	if (prev == NULL) {
		prev = node;
	}
	else {
		prev->next = node;
	}
}
