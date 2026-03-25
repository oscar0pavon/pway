#ifndef PWAY_H
#define PWAY_H

#include <stdbool.h>

#include <wayland-client-protocol.h>


bool pway_create_window(const char* name);

void pway_finish(void);

struct wl_display *pway_display;

#endif
