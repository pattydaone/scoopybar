#include "item.h"

#include "utils/log.h"

#include <stdlib.h>

struct bar_item *
item_init()
{
    struct bar_item *ret = malloc(sizeof(struct bar_item));

    if (ret == nullptr) {
        log_err(__FILE__, __LINE__, "Failed to allocate item.");
        return nullptr;
    }

    return ret;
}
