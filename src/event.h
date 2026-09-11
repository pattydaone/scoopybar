#ifndef EVENT_H
#define EVENT_H

#include "config.h"
#include "bar.h"

struct item_q;

struct event_q {
    char *query;
};

struct bar_msg {
    enum bar_attributes attribute;
    char *value;
};

struct item_m {
    char *name;
    /* TODO: Item attribute, when the time comes */
};

enum event_type {
    EVENT_QUERY,
    EVENT_BAR_MSG,
    EVENT_ITEM_MSG
};

struct event {
    enum event_type type;
    union {
        struct bar_msg *bar_msg;
        struct event_q *query;
    } event;
};

struct event_node {
    struct event *data;
    struct event_node *next;
};

struct event *event_create(enum event_type type, void *event_data);

void event_destroy(struct event *event);

bool empty_queue(struct event_node *queue, struct bar *bar);

#endif
