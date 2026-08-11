#include "config.h"
#include "utils/log.h"
#include "wayland_backend.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const char *valid_bar_keys[]
    = {"height",       "width",          "position", "opacity", "background_color", "margin", "border_width",
       "border_color", "border_opacity", "display",  "layer"};

static const char *valid_sections[] = {"bar", "itemXX"};

bool
extract_color(pixman_color_t *color, char *value, struct bar_ipc *ipc, int cur_line)
{
    char *next;
    int red = strtol(value, &next, 0);
    if (!red && value[0] != '0') {
        char *err = "%s: Invalid red color.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Red value either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        errno = 0;
        return false;
    } else if (red < 0 || red > 255) {
        char *err = "Red value either exceeds 255 or falls short of 0.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }

    int green = strtol(next + 1, &next, 0);
    if (!green) {
        char *err = "%s: Invalid green color.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Green value either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        errno = 0;
        return false;
    } else if (green < 0 || green > 255) {
        char *err = "Green value either exceeds 255 or is less than 0.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }

    int blue = strtol(next + 1, &next, 0);
    if (!blue) {
        char *err = "%s: Invalid blue color.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Blue value either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        errno = 0;
        return false;
    } else if (blue < 0 || blue > 255) {
        char *err = "Blue value either exceeds 255 or is less than 0.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }

    color->red = red * 257;
    color->green = green * 257;
    color->blue = blue * 257;

    return true;
}

bool
set_height(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        char *err = "%s: Invalid bar height.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else 
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err= "Bar height either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else 
            log_conf_err(cur_line, err);
        return false;
    }
    bar->height = val;
    return true;
}

bool
set_width(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        char *err= "%s: Invalid bar width.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else 
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Bar width either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }
    bar->width = val;
    return true;
}

bool
set_pos(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    if (strcmp(value, "top") == 0)
        bar->pos = BAR_TOP;
    else if (strcmp(value, "bottom") == 0)
        bar->pos = BAR_BOTTOM;
    else if (strcmp(value, "left") == 0)
        bar->pos = BAR_LEFT;
    else if (strcmp(value, "right") == 0)
        bar->pos = BAR_RIGHT;
    else {
        char *err = "%s: Invalid bar position.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    }
    return true;
}

bool
set_opacity(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    float val = strtof(value, NULL);
    if (val == 0) {
        char *err = "%s: Invalid background opacity.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
    } else if (errno == ERANGE) {
        char * err = "Bar background opacity either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        errno = 0;
    }
    if (val > 0 && val <= 1.0) {
        bar->opacity = val * 65535;
        return true;
    } else {
        char *err = "Specified value for opacity either exceeds 1 or falls short of 0.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
    }
    return false;
}

bool
set_bg_color(struct bar *bar, char *value, int cur_line)
{
    return extract_color(&bar->background_color, value, bar->ipc, cur_line);
}

bool
set_margin(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        char *err = "%s: Invalid bar margin.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Bar margin either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }
    bar->margin = val;
    return true;
}

bool
set_border_width(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    int val = strtol(value, NULL, 10);
    if (!val && value[0] != '0') {
        char *err = "%s: Invalid bar border width.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
        return false;
    } else if (errno == ERANGE) {
        char *err = "Bar border width either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        return false;
    }
    bar->border.width = val;
    return true;
}

bool
set_border_color(struct bar *bar, char *value, int cur_line)
{
    return extract_color(&bar->border.color, value, bar->ipc, cur_line);
}

bool
set_border_opacity(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    float val = strtof(value, NULL);
    if (val == 0) {
        char *err = "%s: Invalid border opacity.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err, value);
        else
            log_conf_err(cur_line, err, value);
    } else if (errno == ERANGE) {
        char * err = "Border opacity either underflows or overflows.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
        errno = 0;
    }
    if (val > 0 && val <= 1.0) {
        bar->border.color.alpha = val * 65535;
        return true;
    } else {
        char *err = "Specified value for opacity either exceeds 1 or falls short of 0.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
    }
    return false;
}

bool
set_display(struct bar *bar, char *value, int cur_line)
{
    if (strcmp(value, "all") == 0) {
        bar->displays = NULL;
    } else
        strcpy(bar->displays, value);
    return true;
}

bool
set_layer(struct bar *bar, char *value, int cur_line)
{
    struct bar_ipc *ipc = bar->ipc;
    if (strcmp(value, "background") == 0)
        bar->layer = BAR_LAYER_BACKGROUND;
    else if (strcmp(value, "bottom") == 0)
        bar->layer = BAR_LAYER_BOTTOM;
    else if (strcmp(value, "top") == 0)
        bar->layer = BAR_LAYER_TOP;
    else if (strcmp(value, "overlay") == 0)
        bar->layer = BAR_LAYER_OVERLAY;
    else {
        char *err = "Invalid bar layer.";
        if (ipc)
            log_client_err(ipc, __FILE__, __LINE__, err);
        else
            log_conf_err(cur_line, err);
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
        if (!set_height(bar, value, cur_line))
            return false;
    } else
        bar->height = 40;

    /* Bar width */
    if ((find_code = PARSER_find(p, valid_bar_keys[1], value)) == SUCCESS) {
        if (!set_width(bar, value, cur_line))
            return false;
    } else
        bar->width = 0; /* Will be set later at config event */

    /* Bar position */
    if ((find_code = PARSER_find(p, valid_bar_keys[2], value)) == SUCCESS) {
        if (!set_pos(bar, value, cur_line))
            return false;
    } else
        bar->pos = BAR_TOP;

    /* Background opacity */
    if ((find_code = PARSER_find(p, valid_bar_keys[3], value)) == SUCCESS) {
        if (!set_opacity(bar, value, cur_line))
            return false;
    } else
        bar->opacity = 1.0;

    /* Background color */
    if ((find_code = PARSER_find(p, valid_bar_keys[4], value)) == SUCCESS) {
        if (!set_bg_color(bar, value, cur_line))
            return false;
    } else {
        bar->background_color.red = 0;
        bar->background_color.green = 0;
        bar->background_color.blue = 0;
    }
    /* Margin */
    if ((find_code = PARSER_find(p, valid_bar_keys[5], value)) == SUCCESS) {
        if (!set_margin(bar, value, cur_line))
            return false;
    } else
        bar->margin = 0;

    /* Border width */
    if ((find_code = PARSER_find(p, valid_bar_keys[6], value)) == SUCCESS) {
        if (!set_border_width(bar, value, cur_line))
            return false;
    } else
        bar->border.width = 0;

    /* Border color */
    if ((find_code = PARSER_find(p, valid_bar_keys[7], value)) == SUCCESS) {
        if (!set_border_color(bar, value, cur_line))
            return false;
    } else  {
        bar->border.color.red = 65535;
        bar->border.color.green = 65535;
        bar->border.color.blue = 65535;
    }

    /* Border opacity */
    if ((find_code = PARSER_find(p, valid_bar_keys[8], value)) == SUCCESS) {
        if (!set_border_opacity(bar, value, cur_line))
            return false;
    } else
        bar->border.color.alpha = 65535;

    /* Display */
    if ((find_code = PARSER_find(p, valid_bar_keys[9], value)) == SUCCESS) {
        set_display(bar, value, cur_line);
    } else
        bar->displays = NULL;

    /* Bar layer */
    if ((find_code = PARSER_find(p, valid_bar_keys[10], value)) == SUCCESS) {
        if (!set_layer(bar, value, cur_line))
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
check_pos(struct bar *bar, char *value)
{
    if (bar->pos == BAR_TOP || bar->pos == BAR_BOTTOM) {
        if (strcmp(value, "left") == 0 || strcmp(value, "right") == 0) {
            log_client_err(bar->ipc, __FILE__, __LINE__,
                           "Attempted to move bar position from top or bottom to left or right.");
            return false;
        }
    } else if (bar->pos == BAR_LEFT || bar->pos == BAR_RIGHT) {
        if (strcmp(value, "bottom") == 0 || strcmp(value, "top") == 0) {
            log_client_err(bar->ipc, __FILE__, __LINE__,
                           "Attempted to move bar position from right or left to top or bottom.");
            return false;
        }
    }
    return true;
}

bool
bar_set_attribute(struct bar *bar, char *value, enum bar_attributes attr)
{
    switch (attr) {
    case BAR_BACKGROUND_COLOR:
        if (!set_bg_color(bar, value, 0))
            return false;
        bar_refresh_bg_color(bar);
        bar_refresh_border(bar);
        break;
    case BAR_OPACITY:
        if (!set_opacity(bar, value, 0))
            return false;
        bar_refresh_opacity(bar);
        break;
    case BAR_HEIGHT:
        if (!set_height(bar, value, 0))
            return false;
        bar_refresh_height(bar);
        break;
    case BAR_WIDTH:
        if (!set_width(bar, value, 0))
            return false;
        bar_refresh_width(bar);
        break;
    case BAR_POSITION:
        if (!check_pos(bar, value))
            return false;
        if (!set_pos(bar, value, 0))
            return false;
        bar_refresh_position(bar);
        break;
    case BAR_MARGIN:
        if (!set_margin(bar, value, 0))
            return false;
        bar_refresh_margin(bar);
        break;
    case BAR_BORDER_WIDTH:
        if (!set_border_width(bar, value, 0))
            return false;
        break;
    case BAR_BORDER_COLOR:
        if (!set_border_color(bar, value, 0))
            return false;
        break;
    case BAR_BORDER_OPACITY:
        if (!set_border_opacity(bar, value, 0))
            return false;
        break;
    }

    bar_commit(bar);

    return true;
}
