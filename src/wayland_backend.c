#include "wayland_backend.h"
#include "utils/log.h"
#include "bar.h"
#include "ll.h"
#include "wlr-layer-shell-unstable-v1.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

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

    bool busy;  /* Buffer is currently being rendered (=> rendering_buf) */
};

/* TODO: rewrite */
// Taken from wayland-book.com from here...
static void
randname(char *buf)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
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
wl_buffer_release(void *data, struct wl_buffer *wl_buffer)
{
    struct surface_buf *surface_buf = data;
    log_dbg(__FILE__, __LINE__, 3, "Buffer release.");
    surface_buf->busy = false;
}

static const struct wl_buffer_listener wl_buffer_listener = {
    .release = &wl_buffer_release,
};

struct surface_buf *
create_buffer(struct output *output)
{
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
        exit(EXIT_FAILURE);
    }

    struct wl_buffer *wl_buf = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    if (wl_buf == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create wl_buffer.");
        exit(EXIT_FAILURE);
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
    buf->pix = pixman_image_create_bits_no_clear(PIXMAN_a8r8g8b8, width, height, mmapping, stride);
    if (buf->pix == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create pixman image.");
    }

    wl_buffer_add_listener(buf->wl_buf, &wl_buffer_listener, buf);
    log_dbg(__FILE__, __LINE__, 3, "Successfully created buffer and binded to wl_buffer.");

    return buf;
}

bool
destroy_buffer(struct surface_buf *buf)
{
    assert(!buf->busy);

    pixman_image_unref(buf->pix);
    wl_buffer_destroy(buf->wl_buf);
    munmap(buf->map, buf->size);
    return true;
}

void
swap_buffers(struct surface_buf **buf_a, struct surface_buf **buf_b)
{
    struct surface_buf *tmp = *buf_a;
    *buf_a = *buf_b;
    *buf_b = tmp;
}

static bool
resize(struct output *output)
{
    log_dbg(__FILE__, __LINE__, 3, "Resize.");
    struct bar_backend *bar = output->backend;
    enum bar_position bar_pos = bar->bar_frontend->pos;

    zwlr_layer_surface_v1_set_size(output->surface.layer_surface, output->surface.width, output->surface.height);

    zwlr_layer_surface_v1_set_exclusive_zone(output->surface.layer_surface, bar_pos == BAR_TOP || bar_pos == BAR_BOTTOM
                                                                                ? output->surface.height
                                                                                : output->surface.width);

    int margin = output->backend->bar_frontend->margin;
    zwlr_layer_surface_v1_set_margin(output->surface.layer_surface, margin, margin, margin, margin);

    return true;
}

// START: wlr_surface_listener code

static void
zwlr_surface_configure(void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial, uint32_t width,
                       uint32_t height)
{
    log_dbg(__FILE__, __LINE__, 3, "Configure event.");
    struct output *output = data;
    struct bar_backend *bar = output->backend;

    zwlr_layer_surface_v1_ack_configure(surface, serial);

    /* TODO: this might be excessive 
     * perhaps there's a better way 
     * to store this data ? */
    output->surface.height = height;
    output->surface.width = width;
    bar->width = width;
    bar->height = height;
    bar->bar_frontend->width = width;
    bar->bar_frontend->height = height;

    resize(output);
}

static void
zwlr_surface_closed(void *data, struct zwlr_layer_surface_v1 *surface)
{
    // TODO
}

static const struct zwlr_layer_surface_v1_listener zwlr_surface_listener
    = {.configure = &zwlr_surface_configure, .closed = &zwlr_surface_closed};

// END: wlr_surface_listener code

static void
create_surface(struct output *output)
{
    struct bar_backend *bar = output->backend;

    output->surface.height = bar->height;
    output->surface.width = bar->width;

    if (bar->bar_frontend->displays != NULL && strstr(bar->bar_frontend->displays, output->name) != 0) {
        log_err(__FILE__, __LINE__, "%s: Invalid output.", output->name);
        exit(EXIT_FAILURE);
    }

    output->surface.wl_surface = wl_compositor_create_surface(bar->wl_compositor);
    if (output->surface.wl_surface == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create wl_surface for output %s", output->name);
        exit(EXIT_FAILURE);
    }

    enum zwlr_layer_shell_v1_layer layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

    switch (output->backend->bar_frontend->layer) {
    case (BAR_LAYER_BACKGROUND):
        layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
        break;
    case (BAR_LAYER_BOTTOM):
        layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
        break;
    case (BAR_LAYER_OVERLAY):
        layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
        break;
    case (BAR_LAYER_TOP):
        layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
        break;
    }

    output->surface.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        bar->zwlr_layer_shell, output->surface.wl_surface, output->output, layer, "panel");

    if (output->surface.layer_surface == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create layer_surface for output %s", output->name);
        exit(EXIT_FAILURE);
    }

    zwlr_layer_surface_v1_add_listener(output->surface.layer_surface, &zwlr_surface_listener, output);

    enum zwlr_layer_surface_v1_anchor location
        = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

    enum bar_position bar_pos = output->backend->bar_frontend->pos;
    switch (bar_pos) {
    case (BAR_TOP):
        location
            = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        break;
    case (BAR_BOTTOM):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
        break;
    case (BAR_LEFT):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    case (BAR_RIGHT):
        location = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                   | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        break;
    }

    zwlr_layer_surface_v1_set_anchor(output->surface.layer_surface, location);

    log_dbg(__FILE__, __LINE__, 3, "Surface created for output %s", output->name);
}

// START: wl_output_listener code

static void
wl_output_geometry(void *data, struct wl_output *output, int x, int y, int physical_width, int physical_height,
                   int subpixel, const char *make, const char *model, int transform)
{
    struct output *out = data;
    out->transform = transform;
}

static void
wl_output_mode(void *data, struct wl_output *output, uint flags, int width, int height, int refresh)
{
    struct output *out = data;
    out->width = width;
    out->height = height;
}

static void
wl_output_scale(void *data, struct wl_output *output, int scale)
{
    struct output *out = data;
    out->scale = scale;
}

static void
wl_output_name(void *data, struct wl_output *output, const char *name)
{
    struct output *out = data;
    out->name = (name != NULL ? strdup(name) : NULL);
}

static void
wl_output_done(void *data, struct wl_output *output)
{
    struct output *out = data;
    if (!out->scale)
        out->scale = 1;

    create_surface(out);
    resize(out);

    wl_surface_commit(out->surface.wl_surface);

    if (!out->backend->height) {
        log_dbg(__FILE__, __LINE__, 3, "Bar height not specified; defaulting to 40");
        out->backend->height = 40;
        out->backend->bar_frontend->height = out->height;
    }

    if (!out->backend->width) {
        log_dbg(__FILE__, __LINE__, 3, "Bar height not specified; defaulting to length of %s", out->name);
        out->backend->width = out->width;
        out->backend->bar_frontend->width = out->width;
    }

    struct surface_buf *buf_a = create_buffer(out);
    if (buf_a == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", out->name);
        exit(EXIT_FAILURE);
    }
    out->rendering_buf = buf_a;

    struct surface_buf *buf_b = create_buffer(out);
    if (buf_b == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", out->name);
        exit(EXIT_FAILURE);
    }
    out->pending_buf = buf_b;
}

static void
wl_output_description(void *data, struct wl_output *output, const char *description)
{
    // No neeed
}

static const struct wl_output_listener wl_output_listener = {.geometry = &wl_output_geometry,
                                                             .mode = &wl_output_mode,
                                                             .done = &wl_output_done,
                                                             .scale = &wl_output_scale,
                                                             .name = &wl_output_name,
                                                             .description = &wl_output_description};

// END: wl_output_listener code

void
check_version(const char *interface, uint32_t required, uint32_t actual)
{
    if (actual >= required)
        return;

    log_err(__FILE__, __LINE__, "Version for interface %s is %d, where %d is required.", interface, actual, required);
    exit(EXIT_FAILURE);
}

static void
registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
    struct bar_backend *bar = data;
    if (strcmp(interface, wl_shm_interface.name) == 0) {
        check_version(interface, 1, version);

        bar->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
        log_dbg(__FILE__, __LINE__, 3, "Binded to wl_shm.");
    } else if (strcmp(interface, wl_compositor_interface.name) == 0) {
        check_version(interface, 4, version);

        bar->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4);
        log_dbg(__FILE__, __LINE__, 3, "Binded to wl_compositor.");
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        check_version(interface, 3, version);

        bar->zwlr_layer_shell = wl_registry_bind(wl_registry, name, &zwlr_layer_shell_v1_interface, 3);
        log_dbg(__FILE__, __LINE__, 3, "Binded to zwlr_layer_shell.");
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        check_version(interface, 4, version);

        struct output *out = malloc(sizeof(struct output));
        out->output = wl_registry_bind(wl_registry, name, &wl_output_interface, 4);
        log_dbg(__FILE__, __LINE__, 3, "Binded to wl_output.");

        out->backend = bar;

        wl_output_add_listener(out->output, &wl_output_listener, out);
        log_dbg(__FILE__, __LINE__, 3, "Added wl_output listener.");

        if (bar->outputs == NULL) { // TODO: why do i have to do this? fuck you
            struct output_node *node = malloc(sizeof(struct output_node));
            node->data = out;
            node->next = NULL;
            bar->outputs = node;
        } else {
            LL_push_back_output(bar->outputs, out);
        }
    }
}

static void
registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener
    = {.global = &registry_global, .global_remove = &registry_global_remove};

struct bar_backend *
init_bar_backend(struct bar *bar)
{
    struct bar_backend *ret = malloc(sizeof(struct bar_backend));

    if (ret == NULL) {
        return NULL;
    }

    ret->bar_frontend = bar;
    ret->width = bar->width;
    ret->height = bar->height;
    ret->background_color = &bar->background_color;
    ret->background_color->alpha = 65535 * bar->opacity;

    ret->wl_display = wl_display_connect(NULL);
    if (ret->wl_display == NULL) {
        log_err(__FILE__, __LINE__, "Failed to connect to wl_display.");
        goto err;
    }

    ret->wl_registry = wl_display_get_registry(ret->wl_display);
    if (ret->wl_registry == NULL) {
        log_err(__FILE__, __LINE__, "Failed to get wl_registry.");
        goto err;
    }

    wl_registry_add_listener(ret->wl_registry, &registry_listener, ret);
    wl_display_roundtrip(ret->wl_display);

    if (ret->wl_shm == NULL) {
        log_err(__FILE__, __LINE__, "wl_shm not created.");
        goto err;
    }

    if (ret->wl_compositor == NULL) {
        log_err(__FILE__, __LINE__, "Failed to get wl_compositor.");
        goto err;
    }

    if (ret->zwlr_layer_shell == NULL) {
        log_err(__FILE__, __LINE__, "Failed to get layer_shell object.");
        goto err;
    }

    if (ret->outputs == NULL) {
        log_err(__FILE__, __LINE__, "Failed to connect to any outputs.");
        goto err;
    }

    wl_display_roundtrip(ret->wl_display);

    wl_display_roundtrip(ret->wl_display);

    return ret;
err:
    free(ret);
    return NULL;
}

bool
bar_commit(struct bar *bar)
{
    struct bar_backend *backend = bar->backend;
    struct output_node *cur = backend->outputs;
    while (cur != NULL) {
        struct output *output = cur->data;
        struct surface_buf *buf = output->pending_buf;
        assert(buf->busy == false);
        assert(buf->pix != NULL);

        pixman_image_composite(PIXMAN_OP_SRC, bar->pix, NULL, buf->pix, 0, 0, 0, 0, 0, 0, bar->width, bar->height);

        wl_surface_attach(output->surface.wl_surface, buf->wl_buf, 0, 0);
        wl_surface_damage_buffer(output->surface.wl_surface, 0, 0, bar->width, bar->height);
        wl_surface_commit(output->surface.wl_surface);
        wl_display_flush(backend->wl_display);

        buf->busy = true;

        swap_buffers(&output->rendering_buf, &output->pending_buf);

        cur = cur->next;
    }
    wl_display_roundtrip(backend->wl_display);

    return true;
}

bool
resize_surfaces(struct bar *bar)
{
    struct bar_backend *backend = bar->backend;
    backend->height = bar->height;
    backend->width = bar->width;

    struct output_node *cur = backend->outputs;

    while (cur != NULL) {
        struct output *out = cur->data;
        if (bar->height > out->pending_buf->height || bar->width > out->pending_buf->width) {
            destroy_buffer(out->pending_buf);
            out->pending_buf = create_buffer(out);

            bar_commit(bar);

            destroy_buffer(out->pending_buf);
            out->pending_buf = create_buffer(out);
        }
        out->surface.height = bar->height;
        out->surface.width = bar->width;

        resize(cur->data);
        cur = cur->next;
    }

    bar_commit(bar);

    return true;
}
