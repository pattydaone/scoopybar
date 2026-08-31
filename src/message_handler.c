#include "message_handler.h"

#include "ipc.h"
#include "config.h"
#include "utils/log.h"

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
send_all(struct bar *bar)
{
    int written = 0;
    struct bar_ipc *ipc = bar->ipc;
    char to_write[1024];
    int intermediate_written = 0;
    written += snprintf(ipc->msg, 1024 - written, "{\n\t");

    intermediate_written = snprintf(to_write, 1024, "\t\"height\": %d,\n", bar->height);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    stpncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"width\": %d,\n", bar->width);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"opacity\": %f,\n", bar->opacity / 65536.0);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"color\": \"%d,%d,%d\",\n", bar->background_color.red,
                        bar->background_color.green, bar->background_color.blue);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"margin\": %d,\n", bar->margin);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"border_width\": %d,\n", bar->border.width);
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written = snprintf(to_write, 1024 - written, "\t\"displays\": \"%s\",\n",
                        (bar->displays == NULL ? "all" : bar->displays));
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;


    switch (bar->pos) {
    case (BAR_TOP):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"top\",\n");
    case (BAR_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"bottom\",\n");
    case (BAR_LEFT):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"left\",\n");
    case (BAR_RIGHT):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"right\",\n");
    }
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    switch (bar->layer) {
    case (BAR_LAYER_BACKGROUND):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"background\"\n");
    case (BAR_LAYER_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"bottom\"\n");
    case (BAR_LAYER_TOP):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"top\"\n");
    case (BAR_LAYER_OVERLAY):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"overlay\"\n");
    }
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    intermediate_written += snprintf(to_write, 1024, "}");
    if (intermediate_written > 1024 - written) {
        IPC_send_msg(ipc);
        written = 0;
    }
    strncpy(ipc->msg + written, to_write, intermediate_written);
    written += intermediate_written;

    IPC_send_msg(ipc);
}

bool
process_query(struct bar *bar)
{
    char *msg = bar->ipc->msg;
    for (; msg[0] != '.' && msg[0] != '\0'; ++msg)
        ;

    if (msg[0] == '\0') {
        /* Send all */
    }


    return true;
}

bool
process_msg(struct bar *bar)
{
    char *msgs = bar->ipc->msg;
    char type = msgs[0];
    char key[512];
    char value[512];

    if (type == 'm') {
        while ((msgs = extract_kv(bar->ipc, key, value)) != NULL && msgs[0] != '\0')
            if (m_find_by_key(bar, key, value))
                return false;

        if (msgs == NULL)
            return false;

        if (!m_find_by_key(bar, key, value))
            return false;
    }
    else if (type == 'q') {

    }
    else {
        log_client_err(bar->ipc, __FILE__, __LINE__, "Unknown message type.");
        return false;
    }

    bar->ipc->msg_bytes = snprintf(bar->ipc->msg, 1024, "SUCCESS");

    return true;
}
