#include "config.h"

#include "../utils/config_parser.h"

#include <stdlib.h>

struct config *create_configuration(char *file_path, int buf_sz) {
	if (file_path == NULL) return NULL;
	struct ConfParser *conf_file = PARSER_create(file_path, buf_sz);

	struct config *ret = malloc(sizeof(struct config));



	return ret;
}
