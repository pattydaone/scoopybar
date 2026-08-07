#include "config.h"
#include "utils/log.h"
#include "wayland_backend.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *valid_bar_keys[]
    = {"height", "width", "position", "opacity", "background_color", "margin", "border_width", "display", "layer"};

static const char *valid_sections[] = {"bar", "itemXX"};

bool
set_height(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid bar height.", value);
        else 
            log_conf_err(cur_line, "%s: Invalid bar height.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Bar height either underflows or overflows.");
        else 
            log_conf_err(cur_line, "Bar height either underflows or overflows.");
        return false;
    }
    bar->height = val;
    return true;
}

bool
set_width(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid bar width.", value);
        else 
            log_conf_err(cur_line, "%s: Invalid bar width.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Bar width either underflows or overflows.");
        else
            log_conf_err(cur_line, "Bar width either underflows or overflows.");
        return false;
    }
    bar->width = val;
    return true;
}

bool
set_pos(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    if (strcmp(value, "top") == 0)
        bar->pos = BAR_TOP;
    else if (strcmp(value, "bottom") == 0)
        bar->pos = BAR_BOTTOM;
    else if (strcmp(value, "left") == 0)
        bar->pos = BAR_LEFT;
    else if (strcmp(value, "right") == 0)
        bar->pos = BAR_RIGHT;
    else {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid bar position.", value);
        else
            log_conf_err(cur_line, "%s: Invalid bar position.", value);
        return false;
    }
    return true;
}

bool
set_opacity(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    float val = strtof(value, NULL);
    if (val == 0) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid background opacity.", value);
        else
            log_conf_err(cur_line, "%s: Invalid background opacity.", value);
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Bar background opacity either underflows or overflows.");
        else
            log_conf_err(cur_line, "Bar background opacity either underflows or overflows.");
        errno = 0;
    }
    if (val > 0 && val <= 1.0) {
        bar->opacity = val * 65535;
        return true;
    } else {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Specified value for opacity either exceeds 1 or falls short of 0.");
        else
            log_conf_err(cur_line, "Specified value for opacity either exceeds 1 or falls short of 0.");
    }
    return false;
}

bool
set_bg_color(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    char *next;
    int red = strtol(value, &next, 0);
    if (!red && value[0] != '0') {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid red color for bar_background.", value);
        else
            log_conf_err(cur_line, "%s: Invalid red color for bar_background.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Red value for bar_background either underflows or overflows.");
        else
            log_conf_err(cur_line, "Red value for bar_background either underflows or overflows.");
        errno = 0;
        return false;
    } else if (red < 0 || red > 255) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Red value for bar_background either exceeds 255 or falls short of 0.");
        else
            log_conf_err(cur_line, "Red value for bar_background either exceeds 255 or falls short of 0.");
        return false;
    }

    int green = strtol(next + 1, &next, 0);
    if (!green) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid green color for bar_background.", value);
        else
            log_conf_err(cur_line, "%s: Invalid green color for bar_background.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Green value for bar_background either underflows or overflows.");
        else
            log_conf_err(cur_line, "Green value for bar_background either underflows or overflows.");
        errno = 0;
        return false;
    } else if (green < 0 || green > 255) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Green value for bar_background either exceeds 255 or is less than 0.");
        else
            log_conf_err(cur_line, "Green value for bar_background either exceeds 255 or is less than 0.");
        return false;
    }

    int blue = strtol(next + 1, &next, 0);
    if (!blue) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid blue color for bar_background.", value);
        else
            log_conf_err(cur_line, "%s: Invalid blue color for bar_background.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Blue value for bar_background either underflows or overflows.");
        else
            log_conf_err(cur_line, "Blue value for bar_background either underflows or overflows.");
        errno = 0;
        return false;
    } else if (blue < 0 || blue > 255) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Blue value for bar_background either exceeds 255 or is less than 0.");
        else
            log_conf_err(cur_line, "Blue value for bar_background either exceeds 255 or is less than 0.");
        return false;
    }

    bar->background_color.red = red * 257;
    bar->background_color.green = green * 257;
    bar->background_color.blue = blue * 257;
    return true;
}

bool
set_margin(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "%s: Invalid bar margin.", value);
        else
            log_conf_err(cur_line, "%s: Invalid bar margin.", value);
        return false;
    } else if (errno == ERANGE) {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Bar margin either underflows or overflows.");
        else
            log_conf_err(cur_line, "Bar margin either underflows or overflows.");
        return false;
    }
    bar->margin = val;
    return true;
}

bool
set_display(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    if (strcmp(value, "all") == 0) {
        bar->displays = NULL;
    } else
        strcpy(bar->displays, value);
    return true;
}

bool
set_layer(struct bar *bar, char *value, struct bar_ipc *ipc, int cur_line)
{
    if (strcmp(value, "background") == 0)
        bar->layer = BAR_LAYER_BACKGROUND;
    else if (strcmp(value, "bottom") == 0)
        bar->layer = BAR_LAYER_BOTTOM;
    else if (strcmp(value, "top") == 0)
        bar->layer = BAR_LAYER_TOP;
    else if (strcmp(value, "overlay") == 0)
        bar->layer = BAR_LAYER_OVERLAY;
    else {
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, "Invalid bar layer.");
        else
            log_conf_err(cur_line, "Invalid bar layer.");
        return false;
    }
    return true;
}

bool
set_bar_opt(struct bar *bar, struct ConfParser *p)
{
    char value[p->buf_sz];
    enum PARSER_CODES find_code;
    int cur_line = p->current_line;
    /* Bar height */
    if ((find_code = PARSER_find(p, valid_bar_keys[0], value)) == SUCCESS) {
        if (!set_height(bar, value, NULL, cur_line))
            return false;
    } else
        bar->height = 40;

    /* Bar width */
    if ((find_code = PARSER_find(p, valid_bar_keys[1], value)) == SUCCESS) {
        if (!set_width(bar, value, NULL, cur_line))
            return false;
    } else
        bar->width = 0;

    /* Bar position */
    if ((find_code = PARSER_find(p, valid_bar_keys[2], value)) == SUCCESS) {
        if (!set_pos(bar, value, NULL, cur_line))
            return false;
    } else
        bar->pos = BAR_TOP;

    /* Background opacity */
    if ((find_code = PARSER_find(p, valid_bar_keys[3], value)) == SUCCESS) {
        if (!set_opacity(bar, value, NULL, cur_line))
            return false;
    } else
        bar->opacity = 1.0;

    /* Background color */
    if ((find_code = PARSER_find(p, valid_bar_keys[4], value)) == SUCCESS) {
        if (!set_bg_color(bar, value, NULL, cur_line))
            return false;
    } else {
        bar->background_color.red = 0;
        bar->background_color.green = 0;
        bar->background_color.blue = 0;
    }
    /* Margin */
    if ((find_code = PARSER_find(p, valid_bar_keys[5], value)) == SUCCESS) {
        if (!set_margin(bar, value, NULL, cur_line))
            return false;
    } else
        bar->margin = 0;

    /* TODO: Border width */
    if ((find_code = PARSER_find(p, valid_bar_keys[6], value)) == SUCCESS) {
        int val = strtol(value, NULL, 10);
        if (!val && value[0] != '0') {
            log_conf_err(cur_line, "%s: Invalid border width.", value);
            return false;
        } else if (errno == ERANGE) {
            log_conf_err(cur_line, "Border width either underflows or overflows.");
            return false;
        }
        // bar->border_width = val;
    }
    // else bar->border_width = 0;

    /* Display */
    if ((find_code = PARSER_find(p, valid_bar_keys[7], value)) == SUCCESS) {
        set_display(bar, value, NULL, cur_line);
    } else
        bar->displays = NULL;

    /* Bar layer */
    if ((find_code = PARSER_find(p, valid_bar_keys[8], value)) == SUCCESS) {
        if (!set_layer(bar, value, NULL, cur_line))
            return false;
    } else
        bar->layer = BAR_LAYER_BACKGROUND;

    return true;
}

bool
set_opts(struct bar *bar, struct ConfParser *p)
{
    if (strcmp(p->section, valid_sections[0]) == 0) {
        return set_bar_opt(bar, p);
    } else if (strncmp(p->section, valid_sections[1], 4) == 0) {
    }
    return false;
}

bool
bar_set_attribute(struct bar *bar, char *value, enum bar_attributes attr)
{
    switch (attr) {
    case BAR_BACKGROUND_COLOR:
        if (!set_bg_color(bar, value, bar->ipc, 0))
            return false;
        bar_refresh_bg_color(bar);
        bar_commit(bar);
        break;
    case BAR_OPACITY:
        if (!set_opacity(bar, value, bar->ipc, 0))
            return false;
        bar_refresh_opacity(bar);
        bar_commit(bar);
        break;
    case BAR_HEIGHT:
        set_height(bar, value, bar->ipc, 0);
        break;
    case BAR_WIDTH:
        set_width(bar, value, bar->ipc, 0);
        break;
    case BAR_POSITION:
        set_pos(bar, value, bar->ipc, 0);
        break;
    case BAR_MARGIN:
        set_margin(bar, value, bar->ipc, 0);
        break;
    case BAR_BORDER:
        break;
    }

    return true;
}
