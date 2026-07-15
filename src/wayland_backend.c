#include "wayland_backend.h"
#include "ll.h"

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <stdlib.h>

#include <pixman.h>

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
    .release = &wl_buffer_release,
};

static struct wl_buffer *
draw_frame(struct output *state, uint32_t x, uint32_t y)
{
    const int width = state->surface.width;
	const int height = state->surface.height;
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

    struct wl_shm_pool *pool = wl_shm_create_pool(state->backend->wl_shm, fd, size);
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


// START: wlr_surface_listener code

static void zwlr_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height) {
	struct output *state = data;
	zwlr_layer_surface_v1_ack_configure(surface, serial);

	struct wl_buffer *buffer = draw_frame(state, width, height);
    wl_surface_attach(state->surface.wl_surface, buffer, 0, 0);
    wl_surface_commit(state->surface.wl_surface);
}

static void zwlr_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface) {
	// todo
}

static const struct zwlr_layer_surface_v1_listener zwlr_surface_listener = {
	.configure = &zwlr_surface_configure,
	.closed    = &zwlr_surface_closed
};

// END: wlr_surface_listener code


static void create_surface(struct output *output) {
	struct bar_backend *data = output->backend;

	output->surface.height = data->height;
	output->surface.width = data->width;

    output->surface.wl_surface = wl_compositor_create_surface(data->wl_compositor);

	if (output->surface.wl_surface == NULL) {
		fprintf(stderr, "%s:%d \t Failed to create wl_surface for output:  %s\n", __FILE__, __LINE__, output->name);
		exit(1);
	}
	
	output->surface.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			data->zwlr_layer_shell, output->surface.wl_surface, output->output, 
			ZWLR_LAYER_SHELL_V1_LAYER_TOP, "panel");

	if (output->surface.layer_surface == NULL) {
		fprintf(stderr, "%s:%d \t failed to create layer_surface for output:  %s\n", __FILE__, __LINE__, output->name);
		exit(1);
	}

	zwlr_layer_surface_v1_add_listener(output->surface.layer_surface, &zwlr_surface_listener, output);

	zwlr_layer_surface_v1_set_size(output->surface.layer_surface, 
								   data->width, data->height);
	
	zwlr_layer_surface_v1_set_anchor(output->surface.layer_surface, 
			ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);

	zwlr_layer_surface_v1_set_exclusive_zone(output->surface.layer_surface,	
											 output->surface.height);

    wl_surface_commit(output->surface.wl_surface);
}

// START: wl_output_listener code

static void wl_output_geometry(void *data, struct wl_output *output, int x, int y, int physical_width,
							   int physical_height, int subpixel, const char* make, const char* model, int transform) {
	struct output *state = data;
	state->transform = transform;
}

static void wl_output_mode(void *data, struct wl_output *output, uint flags, int width, int height, int refresh) {
	struct output *state = data;
	state->width = width;
	state->height = height;
}

static void wl_output_scale(void *data, struct wl_output *output, int scale) {
	struct output *state = data;
	state->scale = scale;
}

static void wl_output_name(void *data, struct wl_output *output, const char *name) {
	struct output *state = data;
	state->name = (name != NULL ? strdup(name) : NULL);
}

static void wl_output_done(void *data, struct wl_output *output) {
	struct output *state = data;
	if (!state->scale) state->scale = 1;

	create_surface(state);
}

static void wl_output_description(void *data, struct wl_output *output, const char* description) {
	// No neeed
}

static const struct wl_output_listener wl_output_listener = {
	.geometry    = &wl_output_geometry,
	.mode        = &wl_output_mode,
	.done        = &wl_output_done,
	.scale       = &wl_output_scale,
	.name        = &wl_output_name,
	.description = &wl_output_description
};

// END: wl_output_listener code

void check_version(const char *interface, uint32_t required, uint32_t actual) {
	if (actual >= required) return;
	
	fprintf(stderr, "%s:%d\t%s interface is version %d, version %d needed", 
			__FILE__, __LINE__, interface, actual, required);
	exit(1);
}

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
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		check_version(interface, 4, version);

		struct output *out = malloc(sizeof(struct output));
		out->output = wl_registry_bind(wl_registry, name, &wl_output_interface, 4);
		out->backend = state;

		wl_output_add_listener(out->output, &wl_output_listener, out);

		LL_push_back(state->outputs, out, OUTPUT);
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
	ret->width = 1920;
	ret->height = 40;
	ret->wl_display = wl_display_connect(NULL);
	ret->wl_registry = wl_display_get_registry(ret->wl_display);
	wl_registry_add_listener(ret->wl_registry, &registry_listener, ret);
	wl_display_roundtrip(ret->wl_display);

    while (wl_display_dispatch(ret->wl_display)) {
        /* This space deliberately left blank */
    }
	return ret;
}
