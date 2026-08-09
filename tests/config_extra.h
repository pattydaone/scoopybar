#ifndef CONFIG_EXTRA_H
#define CONFIG_EXTRA_H

#include <stdbool.h>
#include <pixman.h>

#include "src/ipc.h"
#include "src/config.h"

bool extract_color(pixman_color_t *color, char *value, struct bar_ipc *ipc, int cur_line);

bool set_height(struct bar *bar, char *value, int cur_line);

bool set_width(struct bar *bar, char *value, int cur_line);

bool set_pos(struct bar *bar, char *value, int cur_line);

bool set_opacity(struct bar *bar, char *value, int cur_line);

bool set_bg_color(struct bar *bar, char *value, int cur_line);

bool set_margin(struct bar *bar, char *value, int cur_line);

bool set_border_width(struct bar *bar, char *value, int cur_line);

bool set_border_color(struct bar *bar, char *value, int cur_line);

bool set_border_opacity(struct bar *bar, char *value, int cur_line);

bool set_display(struct bar *bar, char *value, int cur_line);

bool set_layer(struct bar *bar, char *value, int cur_line);

#endif
