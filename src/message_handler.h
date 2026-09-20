#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "bar.h"
#include "bar_manager.h"

bool process_msg(struct bar *bar);

bool process_message(struct bar_manager *manager);

#endif
