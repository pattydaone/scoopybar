#ifndef WAYLAND_BACKEND_H
#define WAYLAND_BACKEND_H

struct bar_backend {
	struct wl_display *wl_display;
	struct wl_compositor *wl_compositor;
	struct wl_registry *wl_registry;
	struct wl_shm *wl_shm;
	struct xdg_wm_base *xdg_wm_base;

	struct wl_surface *wl_surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
};

struct bar_backend *init_bar_backend();

void init_item_backend();

#endif
