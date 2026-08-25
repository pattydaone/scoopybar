#ifndef LL_H
#define LL_H

#include "include/xdg-output-unstable-v1.h"

#include "wayland_backend.h"

#define ll_foreach(node, cur) \
    for (__typeof__(node) cur = node; cur != NULL; cur = cur->next)

struct output_node {
    struct output *data;
    struct output_node *next;
};

void LL_push_back_output(struct output_node **head, struct output *data);

void LL_delete_output(struct output_node **head, struct output_node *data);

#endif
