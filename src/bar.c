#include "bar.h"
#include <errno.h>
#include <stddef.h>
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

bool set_bar_opt(const char *key, const char *value, struct bar *bar) {
	/* Bar height */
	if (strcmp(key, valid_bar_keys[0]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val) {
			log_err(__FILE__, __LINE__, "Specified value, %s, for key %s was either invalid or zero.", value, key);
		}
		bar->height = val;
		return true;
	}
	/* Bar width */
	else if (strcmp(key, valid_bar_keys[1]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val) {
			log_err(__FILE__, __LINE__, "Specified value, %s, for key %s was either invalid or zero.", value, key);
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Bar width either underflows or overflows.");
			val = 0;
		}
		bar->height = val;
		return true;
	}
	/* Bar position */
	else if (strcmp(key, valid_bar_keys[2]) == 0) {
		if (strcmp(value, "top") == 0) bar->pos = BAR_TOP;
		else if (strcmp(value, "bottom") == 0) bar->pos = BAR_BOTTOM;
		else if (strcmp(value, "left") == 0) bar->pos = BAR_LEFT;
		else if (strcmp(value, "right") == 0) bar->pos = BAR_RIGHT;
		else {
			log_err(__FILE__, __LINE__, "Bar position %s is invalid; defaulting to top", value);
		}
		return true;
	}
	/* Background opacity */
	else if (strcmp(key, valid_bar_keys[3]) == 0) {
		float val = strtof(value, NULL);
		if (val == 0) {
			log_err(__FILE__, __LINE__, "Specified value, %s, for key %s was either invalid or zero.", value, key);
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Specified value, %s, for key %s would either cause an overflow or underflow.", value, key);
			errno = 0;
		}
		if (val > 0 && val <= 1.0) {
			bar->opacity = val;
			return true;
		}
		else {
			log_err(__FILE__, __LINE__, "Specified value for opacity either exceeds 1 or is less than 0.");
		}
		return false;
	}
	/* Background color */
	else if (strcmp(key, valid_bar_keys[4]) == 0) {
		char *next;
		int red = strtol(value, &next, 0);
		if (!red && value[0] != '0') {
			log_err(__FILE__, __LINE__, "%s: Invalid red color for bar background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Red value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (red < 0 || red > 255) {
			log_err(__FILE__, __LINE__, "Red value for bar_background either exceeds 255 or is less than 0.");
			return false;
		}

		int green = strtol(next, &next, 0);
		if (!green && value[0] != '0') {
			log_err(__FILE__, __LINE__, "%s: Invalid green color for bar background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Green value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (green < 0 || green > 255) {
			log_err(__FILE__, __LINE__, "green value for bar_background either exceeds 255 or is less than 0.");
			return false;
		}

		int blue = strtol(next, &next, 0);
		if (!blue && value[0] != '0') {
			log_err(__FILE__, __LINE__, "%s: Invalid blue color for bar background.", value);
			return false;
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Blue value for bar_background either underflows or overflows.");
			errno = 0;
			return false;
		}
		else if (blue < 0 || blue > 255) {
			log_err(__FILE__, __LINE__, "Blue value for bar_background either exceeds 255 or is less than 0.");
			return false;
		}

		bar->background_color.red = red;
		bar->background_color.green = green;
		bar->background_color.blue = blue;
		return true;
	}
	/* Margin */
	else if (strcmp(key, valid_bar_keys[5]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val && value[0] != '0') {
			log_err(__FILE__, __LINE__, "Bar margin was either invalid.");
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Bar margin either underflows or overflows.");
			val = 0;
		}
		bar->margin = val;
		return true;
	}
	/* Border width */ 
	else if (strcmp(key, valid_bar_keys[6]) == 0) {
		int val = strtol(value, NULL, 10);
		if (!val) {
			log_err(__FILE__, __LINE__, "Bar margin was either invalid or zero.");
		}
		else if (errno == ERANGE) {
			log_err(__FILE__, __LINE__, "Bar margin either underflows or overflows.");
			val = 0;
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
			log_err(__FILE__, __LINE__, "Unknown value in bar layer. Defaulting to bottom.");
		}
		return true;
	}
	else {
		log_err(__FILE__, __LINE__, "%s: Unknown key in bar section.");
		return false;
	}
}

bool set_opt(const char *section, const char *key, const char *value, struct bar *bar) {
	if (strcmp(section, valid_sections[0]) == 0) {
		return set_bar_opt(key, value, bar);
	}
	else if (strncmp(section, valid_sections[1], 4) == 0) {
	}
	return false;
}

struct bar *init_bar(struct ConfParser *p) {
	struct bar *ret = malloc(sizeof(struct bar));

	enum PARSER_CODES section_code;
	enum PARSER_CODES kv_code;
	while ((section_code = PARSER_next_section(p)) == SUCCESS) {
		while ((kv_code = PARSER_next_kv(p)) == SUCCESS) {
			set_opt(p->section, p->key, p->value, ret);
		}
	}

	ret->backend = init_bar_backend(ret);

	return ret;
}
