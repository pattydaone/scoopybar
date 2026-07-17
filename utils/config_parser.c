#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ConfParser *PARSER_create(const char *file_path) {
	struct ConfParser *ret = malloc(sizeof(struct ConfParser));
	ret->section = malloc(1);
	ret->key = malloc(1);
	ret->value = malloc(1);

	if ((ret->conf_file = fopen(file_path, "r"))) {
		PARSER_clean(ret);
		return NULL;
	}

	if (PARSER_next_section(ret)) {
		PARSER_clean(ret);
		return NULL;
	}

	return ret;
}

enum PARSER_CODES PARSER_next_section(struct ConfParser *p) {
	int cur;
	while ((cur = fgetc(p->conf_file)) != '[' && cur != EOF);
	
	if (cur == EOF) return END_OF_FILE;

	{
		int buf_sz = 128;
		char tmp_buf[buf_sz]; // Section length shouldnt get super long for my purposes
		int index = 0;
		while ((cur = fgetc(p->conf_file)) != ']' && cur != EOF) {
			if (index == buf_sz) return SECTION_TOO_LONG;
			tmp_buf[index] = cur;
			++index;
		}

		if (cur == EOF) return END_OF_FILE; // TODO: EOF needs to be an error here; in other places its just 
											// a fact that doesnt violate syntax

		free(p->section);
		p->section = strdup(tmp_buf);
	}

	return PARSER_next_kv(p);
}

enum PARSER_CODES PARSER_next_kv(struct ConfParser *p) {
	int cur;
	while ((cur = fgetc(p->conf_file)) != '\n' && cur != EOF);

	if (cur == EOF) return END_OF_FILE;

	{
		int buf_sz = 256;
		char tmp_buf[buf_sz];
		int index = 0;
		while ((cur = fgetc(p->conf_file)) != '=' && cur != EOF) {
			if (index == buf_sz) return SECTION_TOO_LONG;
			tmp_buf[index] = cur;
			++index;
		}

		if (cur == EOF) return END_OF_FILE; // TODO: same as above

		free(p->key);
		p->key = strdup(tmp_buf);
	}

	{
		int buf_sz = 1024;
		char tmp_buf[buf_sz];
		int index = 0;
		while ((cur = fgetc(p->conf_file)) != '\n' && cur != EOF) {
			if (index == buf_sz) return SECTION_TOO_LONG;
			tmp_buf[index] = cur;
			++index;
		}

		if (cur == EOF) return END_OF_FILE; // TODO: same as above

		free(p->value);
		p->value = strdup(tmp_buf);
	}

	return SUCCESS;
}

void PARSER_clean(struct ConfParser *p) {
	fclose(p->conf_file);
	free(p->section);
	free(p->key);
	free(p->value);
	free(p);
}
