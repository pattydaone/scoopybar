#include "message_handler.h"

#include "ipc.h"
#include "config.h"

char *
extract_kv(struct bar_ipc *ipc, char *key, char *value)
{
    char *msgs = ipc->msg;
    /* First two chars determine whether this is a message or query */
    msgs += 2;
    int i = 0;
    char cur;
    while ((cur = msgs[i]) != '=') {
        if (i > 511) {
            snprintf(ipc->msg, 1024, "ERROR: Key too long.");
            return NULL;
        }
        if (cur == ' ' || cur == '\0') {
            snprintf(ipc->msg, 1024, "ERROR: Key without a value.");
            return NULL;
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
            return NULL;
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

bool
m_find_by_key(struct bar *bar, char *key, char *value)
{
    if (strcmp(key, "bar.background") == 0) {
        return bar_set_attribute(bar, value, BAR_BACKGROUND_COLOR);
    }
    else if (strcmp(key, "bar.opacity") == 0) {
        return bar_set_attribute(bar, value, BAR_OPACITY);
    }
    else if (strcmp(key, "bar.height") == 0) {
        return bar_set_attribute(bar, value, BAR_HEIGHT);
    }
    else if (strcmp(key, "bar.width") == 0) {
        return bar_set_attribute(bar, value, BAR_WIDTH);
    }
    else if (strcmp(key, "bar.position") == 0) {
        return bar_set_attribute(bar, value, BAR_POSITION);
    }
    else if (strcmp(key, "bar.margin") == 0) {
        return bar_set_attribute(bar, value, BAR_MARGIN);
    }
    else if (strcmp(key, "bar.border_width") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_WIDTH);
    }
    else if (strcmp(key, "bar.border_color") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_COLOR);
    }
    else if (strcmp(key, "bar.border_opacity") == 0) {
        return bar_set_attribute(bar, value, BAR_BORDER_OPACITY);
    }
    return true;
}

void
process_message()
{
}

bool
process_msg(struct bar *bar)
{
    char *msgs = bar->ipc->msg;
    char type = msgs[0];
    char key[512];
    char value[512];

    while ((msgs = extract_kv(bar->ipc, key, value)) != NULL && msgs[0] != '\0')
        if (type == 'm' && m_find_by_key(bar, key, value))
            return false;

    if (msgs == NULL)
        return false;

    if (type == 'm' && !m_find_by_key(bar, key, value))
        return false;

    bar->ipc->msg_bytes = snprintf(bar->ipc->msg, 1024, "SUCCESS");

    return true;
}
