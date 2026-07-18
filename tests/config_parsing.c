#include "../utils/config_parser.h"
#include <stdio.h>

int main(void) {
	struct ConfParser *p = PARSER_create("/home/patrick/Projects/scoopybar/tests/configurations/conf", 512);
	if (p == NULL) {
		fprintf(stderr, "p failed to initialized idc about the error lo");
	}

	enum PARSER_CODES section_code;
	enum PARSER_CODES kv_code;
	while((section_code = PARSER_next_section(p)) == SUCCESS) {
		printf("[%s]\n", p->section);
		while((kv_code = PARSER_next_kv(p)) == SUCCESS){
			printf("%s=%s\n", p->key, p->value);
		}
		if (kv_code == END_OF_FILE) break;
		printf("\n");
	}

	return 0;
}
