#ifndef CONFIG_H
#define CONFIG_H

#include "utils/config_parser.h"
#include "bar.h"

enum bar_attributes {
    BAR_BACKGROUND_COLOR,
    BAR_OPACITY,
    BAR_HEIGHT,
    BAR_WIDTH,
    BAR_POSITION,
    BAR_MARGIN,
    BAR_BORDER_WIDTH,
    BAR_BORDER_COLOR,
    BAR_BORDER_OPACITY
};

bool set_opts(struct bar *bar, struct ConfParser *p);

bool bar_set_attribute(struct bar *bar, char *value, enum bar_attributes attr);

#endif
