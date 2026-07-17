#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdio.h>

struct ConfParser {
	FILE *conf_file;

	char *section;

	char *key;
	char *value;
};

enum PARSER_CODES {
	END_OF_FILE,
	SUCCESS,
	SECTION_TOO_LONG
};

struct ConfParser *PARSER_create(const char *file_path);

enum PARSER_CODES PARSER_next_section(struct ConfParser *p);

enum PARSER_CODES PARSER_next_kv(struct ConfParser *p);

void PARSER_clean(struct ConfParser *p);

#endif
