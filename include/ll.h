#ifndef LL_H
#define LL_H

#include "xdg-output-unstable-v1.h"
#include <stdlib.h>

struct LLnode {
	struct zxdg_output_v1 *data;
	struct LLnode *next;
};

inline struct LLnode *LL_construct(struct zxdg_output_v1 *output) {
	struct LLnode *ret = malloc(sizeof(struct LLnode));
	ret->data = output;
	ret->next = NULL;
	return ret;
}

inline void LL_push_back(struct LLnode *prev, struct zxdg_output_v1 *output) {
	struct LLnode *node = malloc(sizeof(struct LLnode));
	node->data = output;
	node->next = NULL;

	prev->next = node;
}

// for each ?

#endif
