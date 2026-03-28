#ifndef PWAY_H
#define PWAY_H

#include <stdbool.h>

#include <wayland-client-protocol.h>
#include "mouse.h"
#include <poll.h>

typedef struct PWay{
  PMouse mouse;
  int fd;
  int keys_timer_fd;
  struct pollfd fds[3];
  struct pollfd *events_fds;
  struct pollfd *app_fd;
  void (*resize)(int x,int y);
  void (*exit)(void);
  void (*focus)(bool is_focuses);
  void (*input)(const char* text, int len);
  void (*click)(void);
  void (*click_release)(void);
  void (*update_mouse)(void);
  char* (*get_text)(void);
  
}PWay;

PWay* pway_init();

void pway_set_app_fd(int fd);

bool pway_app_has_event();

bool pway_create_window(const char* name);

void pway_prepare_to_read_events();
void pway_handle_events();

void pway_finish(void);

void pway_primary_copy();

extern PWay* pway;

extern struct wl_display *pway_display;
extern struct wl_surface *pway_surface;

#endif
