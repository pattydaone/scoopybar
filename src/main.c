#include <stdlib.h>

#include "../utils/config_parser.h"
#include "bar.h"

int main(void) {
	struct ConfParser *p = PARSER_create("/home/patrick/Projects/scoopybar/configurations/config.ini", 512);
	if (p == NULL) {
		exit(EXIT_FAILURE);
	}

	struct bar *bar = init_bar(p);
	if (bar == NULL) {
		exit(EXIT_FAILURE);
	}
	free(bar);

	return EXIT_SUCCESS;
}
