#include <stdlib.h>

#include "../utils/config_parser.h"
#include "bar.h"

int main(void) {
	struct ConfParser *p = PARSER_create("/home/patrick/Projects/scoopybar/configurations/config.ini", 512);
	struct bar *bar = init_bar(p);
	free(bar);

	return 0;
}
