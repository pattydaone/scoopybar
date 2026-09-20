#ifndef BAR_MANAGER_H
#define BAR_MANAGER_H

#include "bar.h"
#include "event.h"
#include "wayland_backend.h"

#include "utils/config_parser.h"

struct bar_manager {
    struct queue *queue;
    struct bar *bar;
    struct bar_backend *backend;
};

struct bar_manager *init_bar_manager(struct ConfParser *p);

void destroy_bar_manager(struct bar_manager *manager);

void event_loop(struct bar_manager *manager);

#endif
