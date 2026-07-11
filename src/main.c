#include <stdlib.h>

#include "wayland_backend.h"

int main(void) {
	struct bar_backend *bar = init_bar_backend();
	free(bar);
	return 0;
}
