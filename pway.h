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
  struct pollfd fds[4];
  int paste_fds[2];
  struct pollfd *events_fds;
  struct pollfd *app_fd;
  void (*resize)(int x,int y);
  void (*exit)(void);
  void (*focus)(bool is_focuses);
  void (*input)(const char* text, int len);
  void (*update_keys)(void);
  void (*click)(void);
  void (*click_release)(void);
  void (*update_mouse)(void);
  char* (*get_text)(void);
  int width;
  int height;

  //raw evdev key code and a WL_KEYBOARD_KEY_STATE_*, for both press and
  //release. input() only carries the utf8 of a press, which is not enough to
  //forward keys on to somebody else - a nested compositor needs the code and
  //the release too.
  //INFO new members go on the end: anything compiled against an older pway.h
  //keeps the offsets it was built with, and an inserted field silently moves
  //every one after it
  void (*key)(uint32_t key_code, uint32_t state);

}PWay;

PWay* pway_init();

void pway_set_app_fd(int fd);

bool pway_app_has_event();

bool pway_create_window(const char* name, int width, int height);

void pway_prepare_to_read_events();
void pway_handle_events();

void pway_finish(void);

void pway_egl_resize(int width, int height);

void pway_init_egl();

void pway_swap_buffers();

void pway_primary_copy();

extern PWay* pway;

extern struct wl_display *pway_display;
extern struct wl_surface *pway_surface;

#endif
