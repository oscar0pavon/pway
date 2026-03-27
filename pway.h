#ifndef PWAY_H
#define PWAY_H

#include <stdbool.h>

#include <wayland-client-protocol.h>
#include "mouse.h"

typedef struct PWay{
  PMouse mouse;
  int fd;
  int keys_timer_fd;
}PWay;

PWay* pway_init();

bool pway_create_window(const char* name);

void pway_prepare_to_read_events();
void pway_handle_events();

void pway_finish(void);

void pway_primary_copy();

extern PWay* pway;

struct wl_display *pway_display;
struct wl_surface *pway_surface;

#endif
