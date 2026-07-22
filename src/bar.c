#include "bar.h"
#include <stdlib.h>

struct bar *init_bar() {
	struct bar *ret = malloc(sizeof(struct bar));

	return ret;
}
