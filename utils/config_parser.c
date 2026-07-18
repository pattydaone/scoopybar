#include "config_parser.h"

#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct ConfParser *PARSER_create(const char *file_path, int buf_sz) {
	struct ConfParser *ret = malloc(sizeof(struct ConfParser));
	ret->section = malloc(buf_sz);
	ret->key = malloc(buf_sz);
	ret->value = malloc(buf_sz);
	ret->buf_sz = buf_sz;

	if ((ret->conf_file = fopen(file_path, "r")) == NULL) {
		PARSER_clean(ret);
		return NULL;
	}

	return ret;
}

enum PARSER_CODES PARSER_next_section(struct ConfParser *p) {
	int cur = 0;
	while ((cur = fgetc(p->conf_file)) != '[') {
		if (cur == EOF) return END_OF_FILE;
	}
	

	int index = 0;
	while ((cur = fgetc(p->conf_file)) != ']') {
		if (cur == EOF) log_err(__FILE__, __LINE__, "Expected ], got end of file.");
		else if (cur == '#') log_err(__FILE__, __LINE__, "Expected ], got comment.");

		if (index == p->buf_sz - 2) return SECTION_TOO_LONG;
		p->section[index] = cur;
		++index;
	}
	p->section[index] = '\0';

	while ((cur = fgetc(p->conf_file)) != '\n' && cur != EOF);
	return SUCCESS;
}

enum PARSER_CODES PARSER_next_kv(struct ConfParser *p) {
	int cur = 0;

	int index = 0;
	while ((cur = fgetc(p->conf_file)) != '=') {
		if (cur == EOF) return END_OF_FILE;
		else if (cur == '[') {
			fseek(p->conf_file, ftell(p->conf_file) - 1, SEEK_SET);
			return END_OF_SECTION;
		}
		else if (cur == '#') {
			while (fgetc(p->conf_file) != '\n');
			break;
		}
		else if (cur == ' ' || cur == '\t') continue;


		if (index == p->buf_sz - 2) return SECTION_TOO_LONG;
		p->key[index] = cur;
		++index;
	}
	p->key[index] = '\0';


	index = 0;
	while ((cur = fgetc(p->conf_file)) != '\n') {
		if (cur == EOF) log_err(__FILE__, __LINE__, "Expected value, got end of file.");
		else if (cur == '[') log_err(__FILE__, __LINE__, "Expected value, got section begin");
		else if (cur == '#') {
			while (fgetc(p->conf_file) != '\n');
			break;
		}
		else if (cur == ' ' || cur == '\t') continue;


		if (index == p->buf_sz - 2) return SECTION_TOO_LONG;
		p->value[index] = cur;
		++index;
	}
	p->value[index] = '\0';

	return SUCCESS;
}

void PARSER_clean(struct ConfParser *p) {
	fclose(p->conf_file);
	free(p->section);
	free(p->key);
	free(p->value);
	free(p);
}
