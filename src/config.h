#ifndef CONFIG_H
#define CONFIG_H

struct section {
	char *name;
	char *key;
	char *value;
};

struct config {
	struct section *sections;
};

struct config *create_configuration(char *file_path, int buf_sz);

#endif
