#include "bar.h"
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "wayland_backend.h"
#include "ll.h"
#include "../utils/log.h"

const char *valid_sections[] = {
	"bar",
	"itemXX"
};

const char *valid_bar_keys[] = {
	"height",
	"width",
	"position",
	"opacity",
	"background_color",
	"margin",
	"border_width",
	"display",
	"layer"
};

bool set_bar_opt(const char *key, const char *value, struct bar *bar, int cur_line) {
	/* Bar height */
	if (strcmp(key, valid_bar_keys[0]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val && value[0] != '0') {
			log_conf_err(cur_line, "%s: Invalid bar height.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Bar height either underflows or overflows.");
			return false;
		}
		bar->height = val;
		return true;
	}
	/* Bar width */
	else if (strcmp(key, valid_bar_keys[1]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val && value[0] != '0') {
			log_conf_err(cur_line, "%s: Invalid bar width.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Bar width either underflows or overflows.");
			return false;
		}
		bar->width = val;
		return true;
	}
	/* Bar position */
	else if (strcmp(key, valid_bar_keys[2]) == 0) {
		if (strcmp(value, "top") == 0) bar->pos = BAR_TOP;
		else if (strcmp(value, "bottom") == 0) bar->pos = BAR_BOTTOM;
		else if (strcmp(value, "left") == 0) bar->pos = BAR_LEFT;
		else if (strcmp(value, "right") == 0) bar->pos = BAR_RIGHT;
		else {
			log_conf_err(cur_line, "%s: Invalid bar position.", value);
			return false;
		}
		return true;
	}
	/* Background opacity */
	else if (strcmp(key, valid_bar_keys[3]) == 0) {
		float val = strtof(value, NULL);
		if (val == 0) {
			log_conf_err(cur_line, "%s: Invalid background opacity.", value);
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Bar background opacity either underflows or overflows.");
			errno = 0;
		}
		if (val > 0 && val <= 1.0) {
			bar->opacity = val;
			return true;
		}
		else {
			log_conf_err(cur_line, "Specified value for opacity either exceeds 1 or falls short of 0.");
		}
		return false;
	}
	/* Background color */
	else if (strcmp(key, valid_bar_keys[4]) == 0) {
		char *next;
		int red = strtol(value, &next, 0);
		if (!red && value[0] != '0') {
			log_conf_err(cur_line, "%s: Invalid red color for bar_background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Red value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (red < 0 || red > 255) {
			log_conf_err(cur_line, "Red value for bar_background either exceeds 255 or falls short of 0.");
			return false;
		}

		int green = strtol(next + 1, &next, 0);
		if (!green) {
			log_conf_err(cur_line, "%s: Invalid green color for bar_background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Green value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (green < 0 || green > 255) {
			log_conf_err(cur_line, "Green value for bar_background either exceeds 255 or is less than 0.");
			return false;
		}

		int blue = strtol(next + 1, &next, 0);
		if (!blue) {
			log_conf_err(cur_line, "%s: Invalid blue color for bar_background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Blue value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (blue < 0 || blue > 255) {
			log_conf_err(cur_line, "Blue value for bar_background either exceeds 255 or is less than 0.");
			return false;
		}

		bar->background_color.red = red * 257;
		bar->background_color.green = green * 257;
		bar->background_color.blue = blue * 257;
		return true;
	}
	/* Margin */
	else if (strcmp(key, valid_bar_keys[5]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val && value[0] != '0') {
			log_conf_err(cur_line, "%s: Invalid bar margin.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Bar margin either underflows or overflows.");
			return false;
		}
		bar->margin = val;
		return true;
	}
	/* Border width */ 
	else if (strcmp(key, valid_bar_keys[6]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val && value[0] != '0') {
			log_conf_err(cur_line, "%s: Invalid border width.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_conf_err(cur_line, "Border width either underflows or overflows.");
			return false;
		}
		bar->margin = val;
		return true;
	}
	/* Display */
	else if (strcmp(key, valid_bar_keys[7]) == 0) {
		if (strcmp(value, "all") == 0) {
			bar->displays = NULL;
			return true;
		}
		strcpy(bar->displays, value);
		return true;
	}
	/* Bar layer */
	else if (strcmp(key, valid_bar_keys[8]) == 0) {
		if (strcmp(value, "background") == 0) bar->layer = BAR_LAYER_BACKGROUND;
		else if (strcmp(value, "bottom") == 0) bar->layer = BAR_LAYER_BOTTOM;
		else if (strcmp(value, "top") == 0) bar->layer = BAR_LAYER_TOP;
		else if (strcmp(value, "overlay") == 0) bar->layer = BAR_LAYER_OVERLAY;
		else {
			log_conf_err(cur_line, "Invalid bar layer.");
			return false;
		}
		return true;
	}
	else {
		log_conf_err(cur_line, "%s: Unknown key in bar section.", key);
		return false;
	}
}

bool set_opt(const char *section, const char *key, const char *value, struct bar *bar, int cur_line) {
	if (strcmp(section, valid_sections[0]) == 0) {
		return set_bar_opt(key, value, bar, cur_line);
	}
	else if (strncmp(section, valid_sections[1], 4) == 0) {
	}
	return false;
}

struct bar *init_bar(struct ConfParser *p) {
	struct bar *ret = malloc(sizeof(struct bar));
	ret->opacity = 1.0;

	enum PARSER_CODES section_code;
	enum PARSER_CODES kv_code;
	while ((section_code = PARSER_next_section(p)) == SUCCESS) {
		while ((kv_code = PARSER_next_kv(p)) == SUCCESS) {
			if (!set_opt(p->section, p->key, p->value, ret, p->current_line)) {
				return NULL;
			}
		}
	}

	ret->backend = init_bar_backend(ret);

	return ret;
}
