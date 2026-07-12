#include "wayland_backend.h"

#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <stdlib.h>

/* Temp */ 

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

static void
randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A'+(r&15)+(r&16)*2;
        r >>= 5;
    }
}

static int
create_shm_file(void)
{
    int retries = 100;
    do {
        char name[] = "/wl_shm-XXXXXX";
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static int
allocate_shm_file(size_t size)
{
    int fd = create_shm_file();
    if (fd < 0)
        return -1;
    int ret;
    do {
        ret = ftruncate(fd, size);
    } while (ret < 0 && errno == EINTR);
    if (ret < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    /* Sent by the compositor when it's no longer using this buffer */
    wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = wl_buffer_release,
};

static struct wl_buffer *
draw_frame(struct bar_backend *state, uint32_t x, uint32_t y)
{
    const int width = 1920, height = 40;
    int stride = width * 4;
    int size = stride * height;

    int fd = allocate_shm_file(size);
    if (fd == -1) {
        return NULL;
    }

    uint32_t *data = mmap(NULL, size,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(state->wl_shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0,
            width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    /* Draw checkerboxed background */
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((x + y / 8 * 8) % 16 < 8)
                data[y * width + x] = 0xFF666666;
            else
                data[y * width + x] = 0xFFEEEEEE;
        }
    }

    munmap(data, size);
    wl_buffer_add_listener(buffer, &wl_buffer_listener, NULL);
    return buffer;
}

/* End Temp */

static void zwlr_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t x, uint32_t y, uint32_t serial) {
	struct bar_backend *state = data;
	zwlr_layer_surface_v1_ack_configure(surface, serial);

	struct wl_buffer *buffer = draw_frame(state, x, y);
    wl_surface_attach(state->wl_surface, buffer, 0, 0);
    wl_surface_commit(state->wl_surface);
}

static void zwlr_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface) {
	// todo
}

static const struct zwlr_layer_surface_v1_listener zwlr_surface_listener = {
	.configure = &zwlr_surface_configure,
	.closed    = &zwlr_surface_closed
};

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
	struct bar_backend *state = data;
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		state->zwlr_layer_shell = wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, 1);
	}
	else if (strcmp(interface, zwlr_layer_surface_v1_interface.name) == 0) {
		state->layer_surface = wl_registry_bind(wl_registry, name, &zwlr_layer_surface_v1_interface, 1);
		zwlr_layer_surface_v1_add_listener(state->layer_surface, &zwlr_surface_listener, state);
	}
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {

}

static const struct wl_registry_listener registry_listener = {
	.global = &registry_global,
	.global_remove = &registry_global_remove
};

struct bar_backend *init_bar_backend() {
	struct bar_backend *ret = malloc(sizeof(struct bar_backend));
	ret->wl_display = wl_display_connect(NULL);
	ret->wl_registry = wl_display_get_registry(ret->wl_display);
	wl_registry_add_listener(ret->wl_registry, &registry_listener, ret);
	wl_display_roundtrip(ret->wl_display);

    ret->wl_surface = wl_compositor_create_surface(ret->wl_compositor);
	ret->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			ret->zwlr_layer_shell, ret->wl_surface, NULL, 
			ZWLR_LAYER_SHELL_V1_LAYER_TOP, "panel");

	zwlr_layer_surface_v1_set_size(ret->layer_surface, 1920, 40);
	
	zwlr_layer_surface_v1_set_anchor(ret->layer_surface, 
			ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);

	zwlr_layer_surface_v1_set_exclusive_zone(ret->layer_surface, 40);

    wl_surface_commit(ret->wl_surface);

    while (wl_display_dispatch(ret->wl_display)) {
        /* This space deliberately left blank */
    }
	return ret;
}
