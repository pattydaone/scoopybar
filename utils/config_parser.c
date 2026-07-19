#include "config_parser.h"

#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void strip_whitespace(char *buf, int index) {
	int front_offset = 0;

	while (buf[index - 1] == ' ') --index;
	buf[index] = '\0';

	while (buf[front_offset] == ' ') ++front_offset;
	memmove(buf, buf + front_offset, (index - front_offset + 1) * sizeof(char));
}

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
		if (cur == EOF) {
			log_err(__FILE__, __LINE__, "Expected ], got end of file.");
			return MISSING_TOK_ERR;
		}
		else if (cur == '#') {
			log_err(__FILE__, __LINE__, "Expected ], got comment.");
			return MISSING_TOK_ERR;
		}
		else if (cur == '\n') {
			log_err(__FILE__, __LINE__, "Expected ], got new line.");
			return MISSING_TOK_ERR;
		}

		if (index == p->buf_sz - 2) {
			log_err(__FILE__, __LINE__, "Section length exceeds allocated buffer of size %d.", p->buf_sz);
			return SECTION_TOO_LONG_ERR;
		}
		p->section[index] = cur;
		++index;
	}
	strip_whitespace(p->section, index);

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
			continue;
		}
		else if (cur == '\t' || cur == '\n') continue;


		if (index == p->buf_sz - 2) {
			log_err(__FILE__, __LINE__, "Key length exceeds allocated buffer of size %d.", p->buf_sz);
			return KEY_TOO_LONG_ERR;
		}
		p->key[index] = cur;
		++index;
	}
	strip_whitespace(p->key, index);

	index = 0;
	while ((cur = fgetc(p->conf_file)) != '\n') {
		if (cur == EOF) {
			log_err(__FILE__, __LINE__, "Expected value, got end of file.");
			return MISSING_TOK_ERR;
		}
		else if (cur == '[') {
			log_err(__FILE__, __LINE__, "Expected value, got section begin");
			return MISSING_TOK_ERR;
		}
		else if (cur == '#') {
			while (fgetc(p->conf_file) != '\n');
			break;
		}
		else if (cur == '\t') continue;


		if (index == p->buf_sz - 2) {
			log_err(__FILE__, __LINE__, "Value length exceeds allocated buffer of size %d.", p->buf_sz);
			return VALUE_TOO_LONG_ERR;
		}
		p->value[index] = cur;
		++index;
	}
	if (index == 0) {
		log_err(__FILE__, __LINE__, "Expected value, got new line.");
		return MISSING_TOK_ERR;
	}
	strip_whitespace(p->value, index);

	return SUCCESS;
}

int PARSER_clean(struct ConfParser *p) {
	int ret = fclose(p->conf_file);
	free(p->section);
	free(p->key);
	free(p->value);
	free(p);

	return ret;
}
