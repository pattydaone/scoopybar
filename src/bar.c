#include "bar.h"
#include "config.h"

#include <stdlib.h>

#include "wayland_backend.h"

struct bar *init_bar(struct ConfParser *p) {
	struct bar *ret = malloc(sizeof(struct bar));

	enum PARSER_CODES section_code;
	while ((section_code = PARSER_next_section(p)) == SUCCESS) {
		if (!set_opts(ret, p)) {
			return NULL;
		}
	}

	ret->backend = init_bar_backend(ret);

	return ret;
}
