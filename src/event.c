#include "event.h"

#include "utils/log.h"
#include "utils/ll.h"
#include "config.h"

#include <assert.h>
#include <stdlib.h>

struct event *
event_create(enum event_type type, void *event_data)
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
event_destroy(struct event *event)
{
    assert(event != nullptr);

    if (event->event.bar_msg != nullptr)
        free(event->event.bar_msg);
    if (event->event.query != nullptr)
        free(event->event.query);

    free(event);
}

bool
event_process(struct event *event, struct bar *bar)
{
    assert(event != nullptr);

    switch(event->type) {
        case (EVENT_QUERY):
            break;
        case (EVENT_BAR_MSG):
            return bar_set_attribute(bar, event->event.bar_msg->value, event->event.bar_msg->attribute);
        case (EVENT_ITEM_MSG):
            break;
    }

    return false;
}

void
destroy_events(struct event_node *queue)
{
    /* Not sure about this */
    while (queue != nullptr) {
        event_destroy(queue->data);
        LL_delete_event(&queue, queue);
    }
}

bool
empty_queue(struct event_node *queue, struct bar *bar)
{
    ll_foreach(queue, next)
    {
        event_process(next->data, bar);
    }

    destroy_events(queue);

    return true;
}
