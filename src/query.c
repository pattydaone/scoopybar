#include "query.h"

#include "ipc.h"

int
write_bar_height(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\"height\": %d,", bar->height);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    stpncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_width(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\"width\": %d,", bar->width);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_opacity(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written
        = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\"opacity\": %f,", bar->opacity / 65536.0);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_color(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(
        to_write, 1024 - bar->ipc->msg_bytes, "\"color\": {\"red\": %d,\"green\": %d,\"blue\": %d},",
        bar->background_color.red / 257, bar->background_color.green / 257, bar->background_color.blue / 257);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_margin(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\"margin\": %d,", bar->margin);
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_border(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(
        to_write, 1024 - bar->ipc->msg_bytes,
        "\"border\": {\"width\": %d,\"color\": {\"red\": %d,\"green\": %d,\"blue\": %d}},", bar->border.width,
        bar->border.color.red / 257, bar->border.color.green / 257, bar->border.color.blue / 257);

    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_displays(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written = snprintf(to_write, 1024 - bar->ipc->msg_bytes, "\"displays\": \"%s\",",
                                        (bar->displays == nullptr ? "all" : bar->displays));
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_pos(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written;
    switch (bar->pos) {
    case (BAR_TOP):
        intermediate_written = snprintf(to_write, 1024, "\"position\": \"top\",");
        break;
    case (BAR_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\"position\": \"bottom\",");
        break;
    case (BAR_LEFT):
        intermediate_written = snprintf(to_write, 1024, "\"position\": \"left\",");
        break;
    case (BAR_RIGHT):
        intermediate_written = snprintf(to_write, 1024, "\"position\": \"right\",");
        break;
    }
    if (intermediate_written > 1024 - bar->ipc->msg_bytes) {
        IPC_send_msg(bar->ipc);
        bar->ipc->msg_bytes = 0;
    }
    strncpy(bar->ipc->msg + bar->ipc->msg_bytes, to_write, intermediate_written);

    return intermediate_written;
}

int
write_bar_layer(struct bar *bar)
{
    char to_write[1024];
    int intermediate_written;
    switch (bar->layer) {
    case (BAR_LAYER_BACKGROUND):
        intermediate_written = snprintf(to_write, 1024, "\"layer\": \"background\"");
        break;
    case (BAR_LAYER_BOTTOM):
        intermediate_written = snprintf(to_write, 1024, "\"layer\": \"bottom\"");
        break;
    case (BAR_LAYER_TOP):
        intermediate_written = snprintf(to_write, 1024, "\"layer\": \"top\"");
        break;
    case (BAR_LAYER_OVERLAY):
        intermediate_written = snprintf(to_write, 1024, "\"layer\": \"overlay\"");
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
    ipc->msg_bytes = snprintf(ipc->msg, 1024 - ipc->msg_bytes, "{");

    ipc->msg_bytes += write_bar_height(bar);

    ipc->msg_bytes += write_bar_width(bar);

    ipc->msg_bytes += write_bar_color(bar);

    ipc->msg_bytes += write_bar_opacity(bar);

    ipc->msg_bytes += write_bar_margin(bar);

    ipc->msg_bytes += write_bar_border(bar);

    ipc->msg_bytes += write_bar_displays(bar);

    ipc->msg_bytes += write_bar_pos(bar);

    ipc->msg_bytes += write_bar_layer(bar);

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
