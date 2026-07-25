#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include "bar.h"
#include "../utils/config_parser.h"

bool set_opts(struct bar *bar, struct ConfParser *p);

#endif
