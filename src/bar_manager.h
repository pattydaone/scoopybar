#ifndef BAR_MANAGER_H
#define BAR_MANAGER_H

#include "event.h"
#include "bar.h"
#include "utils/config_parser.h"

struct bar_manager {
    struct event_node *queue;
    struct bar *bar;
};

struct bar_manager *bar_manager_create(struct ConfParser *p);

void bar_manager_destroy(struct bar_manager *manager);

#endif
