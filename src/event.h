#ifndef EVENT_H
#define EVENT_H

#include "bar.h"
#include "config.h"

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

struct queue {
    struct event_node *head;
    struct event_node *tail;
};

struct queue *init_queue();

void destroy_queue(struct queue *queue);

struct event *init_event(enum event_type type, void *event_data);

void destroy_event(struct event *event);

bool process_event(struct queue *q, struct bar *bar);

void append_event(struct queue *q, struct event *event);

bool is_empty(struct queue *q);

#endif
