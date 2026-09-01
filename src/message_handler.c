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

int write_bar_height(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"height\": %d,\n", bar->height);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    stpncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_width(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"width\": %d,\n", bar->width);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_opacity(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"opacity\": %f,\n", bar->opacity / 65536.0);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_color(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"color\": \"%d,%d,%d\",\n", bar->background_color.red,
                        bar->background_color.green, bar->background_color.blue);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_margin(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"margin\": %d,\n", bar->margin);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_border(struct bar *bar) {
    char to_write[1024];
    int intermediate_written
        = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"border\": {\n\t\t\"width\": %d,\n\t\t\"color\": \"%d,%d,%d\"\n\t},\n",
                   bar->border.width, bar->border.color.red, bar->border.color.green, bar->border.color.blue);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_displays(struct bar *bar) {
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\t\"displays\": \"%s\",\n",
                        (bar->displays == NULL ? "all" : bar->displays));
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_pos(struct bar *bar) {
    char to_write[1024];
    int intermediate_written;
    switch (bar->pos) {
    case (BAR_TOP):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"top\",\n");
        break;
    case (BAR_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"bottom\",\n");
        break;
    case (BAR_LEFT):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"left\",\n");
        break;
    case (BAR_RIGHT):
        intermediate_written = snprintf(to_write, 1024, "\t\"position\": \"right\",\n");
        break;
    }
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int write_bar_layer(struct bar *bar) {
    char to_write[1024];
    int intermediate_written;
    switch (bar->layer) {
    case (BAR_LAYER_BACKGROUND):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"background\"\n");
        break;
    case (BAR_LAYER_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"bottom\"\n");
        break;
    case (BAR_LAYER_TOP):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"top\"\n");
        break;
    case (BAR_LAYER_OVERLAY):
        intermediate_written = snprintf(to_write, 1024, "\t\"layer\": \"overlay\"\n");
        break;
    }
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

void
send_all(struct bar *bar)
{
    struct bar_ipc *ipc = bar->ipc;
    ipc->msg_bytes = snprintf(ipc->msg, 1024 - ipc->msg_bytes, "{\n");

    ipc->msg_bytes+= write_bar_height(bar);

    ipc->msg_bytes+= write_bar_width(bar);

    ipc->msg_bytes+= write_bar_opacity(bar);

    ipc->msg_bytes+= write_bar_margin(bar);

    ipc->msg_bytes+= write_bar_border(bar);

    ipc->msg_bytes+= write_bar_displays(bar);

    ipc->msg_bytes+= write_bar_pos(bar);

    ipc->msg_bytes+= write_bar_layer(bar);

    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024, "}%c", '\0');
    if (intermediate_written > 1024 - ipc->msg_bytes) {
        IPC_send_msg(ipc);
        ipc->msg_bytes = 0;
    }
    strncpy(ipc->msg + ipc->msg_bytes, to_write, intermediate_written);
    ipc->msg_bytes += intermediate_written + 1;

    IPC_send_msg(ipc);
}

bool
process_query(struct bar *bar)
{
    char *msg = bar->ipc->msg;
    for (; msg[0] != '.' && msg[0] != '\0'; ++msg)
        ;

    if (msg[0] == '\0') {
        send_all(bar);
        return true;
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
        process_query(bar);
    }
    else {
        log_client_err(bar->ipc, __FILE__, __LINE__, "Unknown message type.");
        return false;
    }

    bar->ipc->msg_bytes = snprintf(bar->ipc->msg, 1024, "SUCCESS") + 1;

    return true;
}
