#include "event.h"

#include "config.h"
#include "utils/log.h"
#include "query.h"

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

    memset(ret, 0, sizeof(struct queue));

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

    if (event->type == EVENT_BAR_MSG && event->event.bar_msg != nullptr) {
        free(event->event.bar_msg->value);
        free(event->event.bar_msg);
    }
    if (event->type == EVENT_QUERY && event->event.query != nullptr) {
        free(event->event.query->query);
        free(event->event.query);
    }

    free(event);
}

bool
process_event(struct queue *queue, struct bar *bar)
{
    assert(queue != nullptr);

    struct event *event = queue->head->data;
    assert(event != nullptr);

    switch (event->type) {
    case (EVENT_QUERY):
        if (!process_query(bar)) {
            log_err(__FILE__, __LINE__, "Failed to process query.");
            return false;
        }
        break;
    case (EVENT_BAR_MSG):
        if (!bar_set_attribute(bar, event->event.bar_msg->value, event->event.bar_msg->attribute)) {
            log_err(__FILE__, __LINE__, "Failed to process message.");
            return false;
        }
        break;
    case (EVENT_ITEM_MSG):
        log_err(__FILE__, __LINE__, "EVENT_ITEM_MSG not yet supported.");
        return false;
        break;
    }

    struct event_node *to_destroy = queue->head;
    if (to_destroy == queue->tail)
        queue->tail = nullptr;
    queue->head = to_destroy->next;
    destroy_event(to_destroy->data);
    free(to_destroy);

    bar->ipc->msg_bytes = snprintf(bar->ipc->msg, 1024, "SUCCESS") + 1;
    if (!IPC_send_msg(bar->ipc)) {
        log_err(__FILE__, __LINE__, "Failed to reply to message.");
        return false;
    }

    return true;
}

void
append_event(struct queue *q, struct event *event)
{
    struct event_node *node = malloc(sizeof(struct event_node));

    node->data = event;
    node->next = nullptr;
    if (q->head == nullptr || q->tail == nullptr)
        q->head = node;
    else
        q->tail->next = node;
    q->tail = node;
}

bool
is_empty(struct queue *q)
{
    return q->head == nullptr;
}
