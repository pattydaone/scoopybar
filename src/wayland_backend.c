#include "wayland_backend.h"
#include "bar.h"
#include "ll.h"
#include "utils/log.h"
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
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>
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
wl_buffer_release(void *data, struct wl_buffer *)
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
    pixman_image_unref(buf->pix);
    wl_buffer_destroy(buf->wl_buf);
    munmap(buf->map, buf->size);
    free(buf);
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
zwlr_surface_closed(void *, struct zwlr_layer_surface_v1 *)
{
    // TODO
}

static const struct zwlr_layer_surface_v1_listener zwlr_surface_listener
    = {.configure = &zwlr_surface_configure, .closed = &zwlr_surface_closed};

// END: wlr_surface_listener code

static bool
create_surface(struct output *output)
{

    struct bar_backend *bar = output->backend;

    if (!output->backend->height) {
        log_info(__FILE__, __LINE__, "Bar height not specified; defaulting to 40");
        bar->height = 40;
        bar->bar_frontend->height = 40;
    }

    if (!output->backend->width) {
        log_info(__FILE__, __LINE__, "Bar height not specified; defaulting to length of %s", output->name);
        output->backend->width = output->width;
        output->backend->bar_frontend->width = output->width;
    }

    output->surface.height = bar->height;
    output->surface.width = bar->width;

    output->surface.wl_surface = wl_compositor_create_surface(bar->wl_compositor);
    if (output->surface.wl_surface == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create wl_surface for output %s", output->name);
        goto err;
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
        goto err;
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

    resize(output);

    wl_surface_commit(output->surface.wl_surface);

    output->rendering_buf = create_buffer(output);
    if (output->rendering_buf == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", output->name);
        goto err;
    }

    output->pending_buf = create_buffer(output);
    if (output->pending_buf == NULL) {
        log_err(__FILE__, __LINE__, "Failed to create buffer for output %s", output->name);
        goto err;
    }

    return true;

err:
    if (output->surface.wl_surface != NULL)
        wl_surface_destroy(output->surface.wl_surface);
    if (output->surface.layer_surface != NULL)
        zwlr_layer_surface_v1_destroy(output->surface.layer_surface);
    if (output->rendering_buf != NULL)
        destroy_buffer(output->rendering_buf);
    if (output->pending_buf != NULL)
        destroy_buffer(output->pending_buf);
    return false;
}

// START: wl_output_listener code

static void
wl_output_geometry(void *data, struct wl_output *, int, int, int, int,
                   int, const char *, const char *, int transform)
{
    struct output *out = data;
    out->transform = transform;
}

static void
wl_output_mode(void *data, struct wl_output *, uint32_t, int width, int height, int)
{
    struct output *out = data;
    out->width = width;
    out->height = height;
}

static void
wl_output_scale(void *data, struct wl_output *, int scale)
{
    struct output *out = data;
    out->scale = scale;
}

static void
wl_output_name(void *data, struct wl_output *, const char *name)
{
    struct output *out = data;
    out->name = (name != NULL ? strdup(name) : NULL);
}

static void
wl_output_done(void *data, struct wl_output *)
{
    struct output *out = data;
    if (!out->scale)
        out->scale = 1;
}

static void
wl_output_description(void *, struct wl_output *, const char *)
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

        LL_push_back_output(&bar->outputs, out);
    }
}

static void
registry_global_remove(void *, struct wl_registry *, uint32_t)
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
    ret->outputs = NULL;

    ret->wl_display = wl_display_connect(NULL);
    if (ret->wl_display == NULL) {
        log_err(__FILE__, __LINE__, "Failed to connect to wl_display.");
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

    char *valid_outputs = ret->bar_frontend->displays;
    for (struct output_node *cur = ret->outputs; cur != NULL;) {
        if (cur->data->name == NULL) {
            struct output_node *tmp = cur;
            cur = cur->next;
            LL_delete_output(&ret->outputs, tmp);
            continue;
        }
        if (valid_outputs != NULL && strstr(valid_outputs, cur->data->name) == NULL) {
            struct output_node *tmp = cur;
            cur = cur->next;
            LL_delete_output(&ret->outputs, tmp);
            continue;
        }
        if (!create_surface(cur->data))
            goto err;
        if (!resize(cur->data))
            goto err;

        cur = cur->next;
    }
    if (ret->outputs == NULL) {
        log_err(__FILE__, __LINE__, "No displays found.");
        goto err;
    }

    wl_display_roundtrip(ret->wl_display);

    return ret;
err:
    destroy_bar_backend(ret);
    return NULL;
}

void
destroy_bar_backend(struct bar_backend *backend)
{
    struct output_node *to_free = NULL;
    for (struct output_node *cur = backend->outputs; cur != NULL; cur = cur->next) {
        if (cur->data != NULL) {
            struct output *output = cur->data;
            if (output->output != NULL) {
                wl_output_destroy(output->output);
            }
            if (output->name != NULL) {
                free(output->name);
            }
            if (output->pending_buf != NULL)
                destroy_buffer(output->pending_buf);
            if (output->rendering_buf != NULL)
                destroy_buffer(output->rendering_buf);
            if (output->surface.layer_surface != NULL) {
                zwlr_layer_surface_v1_destroy(output->surface.layer_surface);
            }
            if (output->surface.wl_surface != NULL) {
                wl_surface_destroy(output->surface.wl_surface);
            }
            free(output);
        }
        if (to_free != NULL) {
            free(to_free);
        }
        to_free = cur;
    }
    if (to_free != NULL)
        free(to_free);

    if (backend->zwlr_layer_shell != NULL) {
        zwlr_layer_shell_v1_destroy(backend->zwlr_layer_shell);
    }

    if (backend->wl_shm != NULL) {
        wl_shm_destroy(backend->wl_shm);
    }

    if (backend->wl_compositor != NULL) {
        wl_compositor_destroy(backend->wl_compositor);
    }
    if (backend->wl_registry != NULL) {
        wl_registry_destroy(backend->wl_registry);
    }
    if (backend->wl_display != NULL) {
        wl_display_disconnect(backend->wl_display);
    }

    free(backend);
}

bool
wayland_event_loop(struct bar *bar)
{
    /* TODO: move all of this to bar_loop and make socketfd
     * pollable so everything is all in one place
     */
    struct bar_backend *backend = bar->backend;

    while (wl_display_prepare_read(backend->wl_display) != 0) {
        if (wl_display_dispatch_pending(backend->wl_display) == -1) {
            log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
            goto err;
        }
    }

    wl_display_flush(backend->wl_display);

    struct pollfd fds[] = {
        { .fd = wl_display_get_fd(backend->wl_display), .events = POLLIN },
    };

    while (true) {
        int ret = poll(fds, sizeof(fds) / sizeof(fds[0]), -1);
        if (ret == -1) {
            log_err(__FILE__, __LINE__, "Failed to poll.");
            goto err;
        }

        if (fds[0].revents & POLLIN) {
            if (wl_display_read_events(backend->wl_display) == -1) {
                log_err(__FILE__, __LINE__, "Failed to read from wayland socket.");
                goto err;
            }

            while (wl_display_prepare_read(backend->wl_display) != 0) {
                if (wl_display_dispatch_pending(backend->wl_display) == -1) {
                    log_err(__FILE__, __LINE__, "Failed to dispatch pending wayland events.");
                    goto err;
                }
            }

            wl_display_flush(backend->wl_display);
        }
    }

    wl_display_cancel_read(backend->wl_display);
    return true;
err:
    wl_display_cancel_read(backend->wl_display);
    return false;
}

bool
bar_commit(struct bar *bar)
{
    struct bar_backend *backend = bar->backend;
    for (struct output_node *cur = backend->outputs; cur != NULL; cur = cur->next) {
        struct output *output = cur->data;
        struct surface_buf *buf = output->pending_buf;
        assert(buf->busy == false);
        assert(buf->pix != NULL);

        pixman_image_composite(PIXMAN_OP_SRC, bar->pix, NULL, buf->pix, 0, 0, 0, 0, 0, 0, bar->width, bar->height);

        wl_surface_attach(output->surface.wl_surface, buf->wl_buf, 0, 0);
        wl_surface_damage_buffer(output->surface.wl_surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(output->surface.wl_surface);
        wl_display_flush(backend->wl_display);

        buf->busy = true;

        swap_buffers(&output->rendering_buf, &output->pending_buf);
    }

    return true;
}

bool
resize_buffers(struct bar *bar)
{
    log_dbg(__FILE__, __LINE__, 3, "Resizing buffers.");
    struct bar_backend *backend = bar->backend;
    backend->height = bar->height;
    backend->width = bar->width;

    for (struct output_node *cur = backend->outputs; cur != NULL; cur = cur->next) {
        struct output *out = cur->data;
        if (bar->height > out->pending_buf->height || bar->width > out->pending_buf->width) {
            destroy_buffer(out->pending_buf);
            out->pending_buf = create_buffer(out);

            bar_commit(bar);

            destroy_buffer(out->pending_buf);
            out->pending_buf = create_buffer(out);
        } else {
            /* Clear buffers */
            memset(out->pending_buf->map, 0, out->pending_buf->size);

            bar_commit(bar);

            memset(out->pending_buf->map, 0, out->pending_buf->size);
        }
        out->surface.height = bar->height;
        out->surface.width = bar->width;

        resize(cur->data);
    }

    bar_commit(bar);

    return true;
}
