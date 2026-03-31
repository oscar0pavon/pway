#include "pway.h"
#include <stdlib.h>
#include <stdbool.h>
#include <error.h>
#include "egl.h"
#include "wayland.h"
#include "keyboard.h"
#include "copy_paste.h"

PWay* pway;

struct wl_display *pway_display;
struct wl_surface *pway_surface;



void pway_init_egl(){
  init_egl();
}

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
  struct pollfd paste_event = {-1, POLLIN, 0};

  pway->fds[0] = wayland_poll;
  pway->fds[1] = keyboard_timer_poll;
  pway->fds[2] = app_poll;
  pway->fds[3] = paste_event;

  pway->app_fd = &pway->fds[2];

  return pway;  
}

bool pway_create_window(const char* name, int width, int height){

  xdg_toplevel_set_title(wayland.window, name);
  pway->width = width;
  pway->height = height;

  return true;

}

void pway_handle_events(){

  pway_prepare_to_read_events();

  if (poll(pway->fds, 4, -1) == -1) {
    perror("Poll in fds, APP or Wayland, Keyboard timer");
  }

  if(pway->fds[0].revents & POLLIN){
    wl_display_read_events(wayland.display);
    //printf("wayland poll\n");
  }else{
    wl_display_cancel_read(wayland.display);
  }

  wl_display_dispatch_pending(wayland.display);

  handle_repeat_keys();

   
  pway_can_paste();

  clean_mouse_buttons(); 

}
void pway_finish(void){

  wl_display_disconnect(wayland.display);
}

