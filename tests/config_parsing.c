#include "../utils/config_parser.h"
#include <assert.h>
#include <string.h>

void test1() {
	struct ConfParser *p = PARSER_create("./configurations/complete", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section1") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "blaoh") == 0 );
	assert( strcmp(p->value, "val") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "imjust") == 0 );
	assert( strcmp(p->value, "doingstuff") == 0 );
	assert( code == SUCCESS );


	code = PARSER_next_section(p);
	assert( strcmp(p->section, "anothersection") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "anotherkey") == 0 );
	assert( strcmp(p->value, "wow!!") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "onemore") == 0 );
	assert( strcmp(p->value, "finally") == 0 );
	assert( code == SUCCESS );


	code = PARSER_next_section(p);
	assert( strcmp(p->section, "jkonemore") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "tada") == 0 );
	assert( strcmp(p->value, "voila") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "and....") == 0 );
	assert( strcmp(p->value, "done") == 0 );
	assert( code == SUCCESS );

	assert( PARSER_clean(p) == 0 );
}

void test2() {
	struct ConfParser *p = PARSER_create("./configurations/comments", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "bar") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "key") == 0 );
	assert( strcmp(p->value, "value") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "key") == 0 );
	assert( strcmp(p->value, "value") == 0 );
	assert( code == SUCCESS );


	code = PARSER_next_section(p);
	assert( strcmp(p->section, "bar2") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "key") == 0 );
	assert( strcmp(p->value, "valueagain") == 0 );
	assert( code == SUCCESS );

	assert( PARSER_clean(p) == 0 );
}

void test3() {
	struct ConfParser *p = PARSER_create("./configurations/whitespace", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "heres") == 0 );
	assert( strcmp(p->value, "one with a bunch of     whitespace") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "i hope") == 0 );
	assert( strcmp(p->value, "this is ok") == 0 );
	assert( code == SUCCESS );


	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section again") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "tee+ hee + colon") == 0 );
	assert( strcmp(p->value, "three") == 0 );
	assert( code == SUCCESS );

	assert( PARSER_clean(p) == 0 );
}

void test4() {
	struct ConfParser *p = PARSER_create("./configurations/section_eof", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section1") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "key") == 0 );
	assert( strcmp(p->value, "value") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_section(p);
	assert( code == MISSING_TOK_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test5() {
	struct ConfParser *p = PARSER_create("./configurations/section_comment", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section1") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( strcmp(p->key, "key") == 0 );
	assert( strcmp(p->value, "value") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_section(p);
	assert( code == MISSING_TOK_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test6() {
	struct ConfParser *p = PARSER_create("./configurations/value_eof", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section1") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( code == MISSING_TOK_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test7() {
	struct ConfParser *p = PARSER_create("./configurations/value_eof", 512);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( strcmp(p->section, "section1") == 0 );
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( code == MISSING_TOK_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test8() {
	struct ConfParser *p = PARSER_create("./configurations/section_too_long", 8);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( code == SECTION_TOO_LONG_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test9() {
	struct ConfParser *p = PARSER_create("./configurations/key_too_long", 8);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( code == KEY_TOO_LONG_ERR );

	assert( PARSER_clean(p) == 0 );
}

void test10() {
	struct ConfParser *p = PARSER_create("./configurations/value_too_long", 8);
	assert( p != NULL );

	enum PARSER_CODES code;

	code = PARSER_next_section(p);
	assert( code == SUCCESS );

	code = PARSER_next_kv(p);
	assert( code == VALUE_TOO_LONG_ERR );

	assert( PARSER_clean(p) == 0 );
}

int main(void) {
	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7();
	test8();
	test9();
	test10();

	return 0;
}
