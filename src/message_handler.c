#include "message_handler.h"

#include "config.h"
#include "ipc.h"

#include <stdlib.h>

char *
extract_kv(struct bar_ipc *ipc, char *key, char *value)
{
    char *msgs = ipc->msg;
    /* First two chars determine whether 
     * this is a message or query */
    msgs += 2;
    int i = 0;
    char cur;
    while ((cur = msgs[i]) != '=') {
        if (i > 511) {
            snprintf(ipc->msg, 1024, "ERROR: Key too long.");
            return nullptr;
        }
        if (cur == ' ' || cur == '\0') {
            snprintf(ipc->msg, 1024, "ERROR: Key without a value.");
            return nullptr;
        }
        key[i] = cur;
        ++i;
    }
    key[i] = '\0';
    msgs += i + 1;

    int j = 0;
    while ((cur = msgs[j]) != ' ' && cur != '\0') {
        if (j > 511) {
            snprintf(ipc->msg, 1024, "ERROR: Value too long.");
            return nullptr;
        }
        value[j] = cur;
        ++j;
    }
    value[j] = '\0';
    msgs += j;
    if (msgs[0] == '\0')
        return msgs;
    else
        return msgs + 1;
}

enum bar_attributes m_tokenize(char *key) {
    if (strcmp(key, "bar.background") == 0) {
        return BAR_BACKGROUND_COLOR;
    } else if (strcmp(key, "bar.opacity") == 0) {
        return BAR_OPACITY;
    } else if (strcmp(key, "bar.height") == 0) {
        return BAR_HEIGHT;
    } else if (strcmp(key, "bar.width") == 0) {
        return BAR_WIDTH;
    } else if (strcmp(key, "bar.position") == 0) {
        return BAR_POSITION;
    } else if (strcmp(key, "bar.margin") == 0) {
        return BAR_MARGIN;
    } else if (strcmp(key, "bar.border_width") == 0) {
        return BAR_BORDER_WIDTH;
    } else if (strcmp(key, "bar.border_color") == 0) {
        return BAR_BORDER_COLOR;
    } else if (strcmp(key, "bar.border_opacity") == 0) {
        return BAR_BORDER_OPACITY;
    }

    return BAR_ATTRIBUTE_NULL;
}

bool
process_message(struct bar_manager *manager)
{
    struct bar_ipc *ipc = manager->bar->ipc;
    char *msgs = ipc->msg;
    char type = msgs[0];
    char key[512];
    char value[512];

    if (type == 'm') {
        while ((msgs = extract_kv(ipc, key, value)) != nullptr && msgs[0] != '\0') {
            struct bar_msg *msg = malloc(sizeof(struct bar_msg));
            msg->attribute = m_tokenize(key);
            msg->value = strdup(value);
            struct event *event = init_event(EVENT_BAR_MSG, msg);
            append_event(manager->queue, event);
        }

        if (msgs == nullptr)
            return false;

        struct bar_msg *msg = malloc(sizeof(struct bar_msg));
        msg->attribute = m_tokenize(key);
        msg->value = strdup(value);
        struct event *event = init_event(EVENT_BAR_MSG, msg);
        append_event(manager->queue, event);
    }
    else if (type == 'q') {
        struct event_q *q = malloc(sizeof(struct event_q));
        q->query = strdup(ipc->msg);
        struct event *event = init_event(EVENT_QUERY, q);
        append_event(manager->queue, event);
    }

    return true;
}
