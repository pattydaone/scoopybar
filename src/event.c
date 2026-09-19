#include "event.h"

#include "config.h"
#include "utils/log.h"

#include <assert.h>
#include <stdlib.h>

struct queue *
init_queue()
{
    struct queue *ret = malloc(sizeof(struct queue));
    if (ret == nullptr) {
        log_err(__FILE__, __LINE__, "Failed to create queue object.");
        return nullptr;
    }

    ret->head = nullptr;
    ret->tail = nullptr;

    return ret;
}

void
destroy_queue(struct queue *queue)
{
    assert(queue != nullptr);
    while (queue->head != nullptr) {
        struct event_node *to_destroy = queue->head;
        queue->head = to_destroy->next;
        destroy_event(to_destroy->data);
        free(to_destroy);
    }
    free(queue);
}

struct event *
init_event(enum event_type type, void *event_data)
{
    assert(event_data != nullptr);

    struct event *ret = malloc(sizeof(struct event));
    if (ret == nullptr) {
        log_err(__FILE__, __LINE__, "Failed to allocate event.");
        return nullptr;
    }
    ret->type = type;

    switch (type) {
    case (EVENT_QUERY):
        ret->event.query = event_data;
        break;
    case (EVENT_BAR_MSG):
        ret->event.bar_msg = event_data;
        break;
    case (EVENT_ITEM_MSG):
        break;
    }

    return ret;
}

void
destroy_event(struct event *event)
{
    assert(event != nullptr);

    if (event->event.bar_msg != nullptr)
        free(event->event.bar_msg);
    if (event->event.query != nullptr)
        free(event->event.query);

    free(event);
}

bool
process_event(struct queue *queue, struct bar *bar)
{
    assert(queue != nullptr);

    struct event *event = queue->head->data;
    assert(event != nullptr);

    bool ret = false;
    switch (event->type) {
    case (EVENT_QUERY):
        break;
    case (EVENT_BAR_MSG):
        ret = bar_set_attribute(bar, event->event.bar_msg->value, event->event.bar_msg->attribute);
    case (EVENT_ITEM_MSG):
        break;
    }

    struct event_node *to_destroy = queue->head;
    if (to_destroy == queue->tail)
        queue->tail = nullptr;
    queue->head = to_destroy->next;
    destroy_event(to_destroy->data);
    free(to_destroy);

    return ret;
}

void
append_event(struct queue *q, struct event *event)
{
    struct event_node *node = malloc(sizeof(struct event_node));

    node->data = event;
    node->next = nullptr;
    q->tail->next = node;
    q->tail = node;
}

bool
is_empty(struct queue *q)
{
    return q->head == nullptr;
}
