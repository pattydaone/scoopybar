#ifndef WAYLAND_H
#define WAYLAND_H

#include <wayland-client.h>
#include <signal.h>

void my_round_trip(struct wl_display *display);

bool check_sigint();

#endif
