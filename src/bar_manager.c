#include "bar_manager.h"

#include "bar.h"

#include <stdlib.h>

struct bar_manager *
bar_manager_create(struct ConfParser *p)
{
    struct bar_manager *ret = malloc(sizeof(struct bar_manager));

    ret->bar = init_bar(p);
    ret->queue = nullptr;

    return ret;
}

void
bar_manager_destroy(struct bar_manager *manager)
{
    bar_destroy(manager->bar);
    free(manager);
}
