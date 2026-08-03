#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdio.h>

struct ConfParser {
    int buf_sz;

    FILE *conf_file;

    char *section;

    char *key;
    char *value;

    int current_line;
    fpos_t cur_section_pos;
};

enum PARSER_CODES {
    END_OF_FILE,
    SUCCESS,
    END_OF_SECTION,
    SECTION_TOO_LONG_ERR,
    KEY_TOO_LONG_ERR,
    VALUE_TOO_LONG_ERR,
    MISSING_TOK_ERR,
    KEY_NOT_FOUND,
    GENERAL_ERROR
};

struct ConfParser *PARSER_create(const char *file_path, int buf_sz);

enum PARSER_CODES PARSER_find(struct ConfParser *p, const char *key, char *des);

enum PARSER_CODES PARSER_next_section(struct ConfParser *p);

enum PARSER_CODES PARSER_next_kv(struct ConfParser *p);

int PARSER_clean(struct ConfParser *p);

#endif
