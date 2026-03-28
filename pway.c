#include "pway.h"
#include "wayland.h"
#include <stdlib.h>
#include <stdbool.h>

PWay* pway;

struct wl_display *pway_display;
struct wl_surface *pway_surface;

bool pway_app_has_event(){
  
  return pway->fds[2].revents & POLLIN;

}

void pway_set_app_fd(int fd){
  pway->fds[2].fd = fd;
}


PWay* pway_init(){

  pway = malloc(sizeof(PWay));

  bool status = init_wayland();
  
  pway_display = wayland.display;
  pway_surface = wayland.wayland_surface;


  struct pollfd wayland_poll = {pway->fd, POLLIN, 0};
  struct pollfd keyboard_timer_poll= {pway->keys_timer_fd, POLLIN, 0};
  struct pollfd app_poll = {-1, POLLIN, 0};

  pway->fds[0] = wayland_poll;
  pway->fds[1] = keyboard_timer_poll;
  pway->fds[2] = app_poll;

  pway->app_fd = &pway->fds[2];

  return pway;  
}

bool pway_create_window(const char* name){

  xdg_toplevel_set_title(wayland.window, name);

  return true;

}

void pway_finish(void){

  wl_display_disconnect(wayland.display);
}

