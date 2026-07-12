#ifndef WAYLAND_BACKEND_H
#define WAYLAND_BACKEND_H

#include "../include/wlr-layer-shell-unstable-v1.h"

struct bar_backend {
	struct wl_display *wl_display;
	struct wl_compositor *wl_compositor;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct zwlr_layer_shell_v1 *zwlr_layer_shell;

	struct wl_surface *wl_surface;
	struct zwlr_layer_surface_v1 *layer_surface;
};

struct bar_backend *init_bar_backend();

void init_item_backend();

#endif
