#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "utils/config_parser.h"
#include "bar.h"

enum bar_attributes {
    BAR_BACKGROUND_COLOR,
    BAR_OPACITY,
    BAR_HEIGHT,
    BAR_WIDTH,
    BAR_POSITION,
    BAR_MARGIN,
    BAR_BORDER
};

bool set_opts(struct bar *bar, struct ConfParser *p);

void bar_set_attribute(struct bar *bar, char *value, enum bar_attributes attr);

#endif
