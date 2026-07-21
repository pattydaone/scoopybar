#include "wayland_backend.h"
#include "ll.h"
#include "../utils/log.h"

#include <math.h>
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

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

struct surface_buf {
	struct wl_buffer *wl_buf;
	
	void *map;
	int shm_fd;

	uint32_t width;
	uint32_t height;
	uint32_t size;

	pixman_image_t *pix;

	bool busy;
};

// Taken from wayland-book.com from here...
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
        char name[] = "/bar-shm-XXXXXX";
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
// To here

static void
wl_buffer_release(void *data, struct wl_buffer *wl_buffer) {
	struct surface_buf *state = data;
	log_dbg(__FILE__, __LINE__, 4, "Buffer release.");
	state->busy = false;
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = &wl_buffer_release,
};

struct surface_buf *create_buffer(struct output *output) {
	struct bar_backend *bar = output->backend;
	int height = bar->height;
	int width = bar->width;
	/* BPP is bits per pixel, and stride requires bytes, so we divide by 8.
	 * a8r8g8b8 is divisible by 8, so we need not account for loss of remainder
	 */ 
	int stride = width * PIXMAN_FORMAT_BPP(PIXMAN_a8r8g8b8) / 8;
	int size = 2 * height * stride;

	int fd = allocate_shm_file(size);
	if (fd == -1) {
		log_err(__FILE__, __LINE__, "Failed to allocate shared memory file");
		return NULL;
	}

	uint32_t *mmapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (mmapping == MAP_FAILED) {
		log_err(__FILE__, __LINE__, "Failed to create memory map.");
		close(fd);
		return NULL;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(bar->wl_shm, fd, size);
	if (pool == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create shared memory pool.");
		exit(1);
	}

	struct wl_buffer *wl_buf = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	if (wl_buf == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create wl_buffer.");
		exit(1);
	}

	wl_shm_pool_destroy(pool);
	close(fd);

	struct surface_buf *buf = malloc(sizeof(struct surface_buf));
	buf->height = height;
	buf->width = width;
	buf->size = size;
	buf->map = mmapping;
	buf->wl_buf = wl_buf;
	buf->busy = false;
	buf->pix = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, mmapping, stride);
	if (buf->pix == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create pixman image.");
	}

	wl_buffer_add_listener(buf->wl_buf, &wl_buffer_listener, buf);
	log_dbg(__FILE__, __LINE__, 3, "Successfully created buffer and binded to wl_buffer.");

	return buf;
}

bool commit(struct output *out);

static const struct wl_callback_listener wl_callback_listener;

static void wl_callback_done(void *data, struct wl_callback *wl_cb, uint32_t cb_data) {
	log_dbg(__FILE__, __LINE__, 4, "Frame callback.");
	struct output *output = data;
	assert( output->pending_buf->busy == false );
	
	wl_callback_destroy(wl_cb);

	output->cb = wl_surface_frame(output->surface.wl_surface);
	wl_callback_add_listener(output->cb, &wl_callback_listener, output);
	wl_display_flush(output->backend->wl_display);

	log_dbg(__FILE__, __LINE__, 4, "Buffer not busy.");
	output->color_offset = cb_data - output->color_offset;
	commit(output);

	output->pending_buf->busy = true;

	struct surface_buf *tmp = output->rendering_buf;
	output->rendering_buf = output->pending_buf;
	output->pending_buf = tmp;

	wl_surface_attach(output->surface.wl_surface, output->rendering_buf->wl_buf, 0, 0);
	wl_surface_damage_buffer(output->surface.wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->surface.wl_surface);
	wl_display_flush(output->backend->wl_display);

	output->color_offset = cb_data;
}

static const struct wl_callback_listener wl_callback_listener = {
	.done = &wl_callback_done
};


// START: wlr_surface_listener code

static void zwlr_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width, uint32_t height) {
	struct output *state = data;
	zwlr_layer_surface_v1_ack_configure(surface, serial);

	assert( state->pending_buf != NULL && state->rendering_buf != NULL );

	struct surface_buf *buffer = state->rendering_buf;

	pixman_color_t initial = { 26214, 13107, 39321, 0xffff };
	state->color = initial;
	pixman_image_t *fill = pixman_image_create_solid_fill(&state->color);

	pixman_image_composite(PIXMAN_OP_SRC, fill, NULL, buffer->pix, 0, 0, 0, 0, 0, 0, buffer->width, buffer->height);

	log_dbg(__FILE__, __LINE__, 3, "Surface object for output %s configured.", state->name);

	state->cb = wl_surface_frame(state->surface.wl_surface);
	if (state->cb == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create callback object for output %s.", state->name);
		exit(1);
	}
	wl_callback_add_listener(state->cb, &wl_callback_listener, state);

    wl_surface_attach(state->surface.wl_surface, buffer->wl_buf, 0, 0);
	wl_surface_damage_buffer(state->surface.wl_surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(state->surface.wl_surface);

	wl_display_flush(state->backend->wl_display);
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
		log_err(__FILE__, __LINE__, "Failed to create wl_surface for output %s", output->name);
		exit(1);
	}
	
	output->surface.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			data->zwlr_layer_shell, output->surface.wl_surface, output->output, 
			ZWLR_LAYER_SHELL_V1_LAYER_TOP, "panel");

	if (output->surface.layer_surface == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create layer_surface for output %s", output->name);
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
	log_dbg(__FILE__, __LINE__, 3, "Surface created for output %s", output->name);
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

	struct surface_buf *buf_a = create_buffer(state);
	if (buf_a == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", state->name);
		exit(1);
	}
	state->rendering_buf = buf_a;

	struct surface_buf *buf_b = create_buffer(state);
	if (buf_b == NULL) {
		log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", state->name);
		exit(1);
	}
	state->pending_buf = buf_b;

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
	
	log_err(__FILE__, __LINE__, "Version for interface %s is %d, where %d is required.", interface, actual, required);
	exit(1);
}

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
	struct bar_backend *state = data;
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		check_version(interface, 1, version);

		state->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
		log_dbg(__FILE__, __LINE__, 3, "Binded to wl_shm.");
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		check_version(interface, 4, version);

		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
		log_dbg(__FILE__, __LINE__, 3, "Binded to wl_compositor.");
	}
	else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
		check_version(interface, 3, version);

		state->zwlr_layer_shell = wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, 3);
		log_dbg(__FILE__, __LINE__, 3, "Binded to zwlr_layer_shell.");
	}
	else if (strcmp(interface, wl_output_interface.name) == 0) {
		check_version(interface, 4, version);

		struct output *out = malloc(sizeof(struct output));
		out->output = wl_registry_bind(wl_registry, name, &wl_output_interface, 4);
		log_dbg(__FILE__, __LINE__, 3, "Binded to wl_output.");

		out->backend = state;

		wl_output_add_listener(out->output, &wl_output_listener, out);
		log_dbg(__FILE__, __LINE__, 3, "Added wl_output listener.");

		LL_push_back(state->outputs, out, OUTPUT);
	}
}

static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {

}

static const struct wl_registry_listener registry_listener = {
	.global = &registry_global,
	.global_remove = &registry_global_remove
};

bool commit(struct output *out) {
	log_dbg(__FILE__, __LINE__, 4, "Function \"commit\" called.");

	out->color.red += 10*out->color_offset;
	out->color.green += 10*out->color_offset;
	out->color.blue += 10*out->color_offset;

	pixman_image_t *fill = pixman_image_create_solid_fill(&out->color);

	struct surface_buf *buf = out->pending_buf;
	pixman_image_composite(PIXMAN_OP_SRC, fill, NULL, buf->pix, 0, 0, 0, 0, 0, 0, buf->width, buf->height);

	return true;
}

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
