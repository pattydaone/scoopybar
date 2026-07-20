#ifndef WAYLAND_BACKEND_H
#define WAYLAND_BACKEND_H

#include "../include/wlr-layer-shell-unstable-v1.h"
#include "../include/xdg-output-unstable-v1.h"

#include <stdint.h>

struct output {
	struct wl_output *output;
	uint32_t width;
	uint32_t height;
	uint32_t scale;
	uint32_t transform;
	char *name;

	struct bar_backend *backend;

	struct {
		struct wl_surface *wl_surface;
		struct zwlr_layer_surface_v1 *layer_surface;

		uint32_t height;
		uint32_t width;
	} surface;
};

struct bar_backend {
	struct wl_display *wl_display;
	struct wl_compositor *wl_compositor;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct zwlr_layer_shell_v1 *zwlr_layer_shell;
	struct wl_callback *cb;

	struct output_node *outputs;

	uint32_t width;
	uint32_t height;

	struct bar_buf *buf;
};

struct bar_backend *init_bar_backend();

void init_item_backend();

#endif
