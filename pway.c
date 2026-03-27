#include "pway.h"
#include "wayland.h"
#include <stdlib.h>
#include <stdbool.h>

PWay* pway;

struct wl_display *pway_display;
struct wl_surface *pway_surface;

PWay* pway_init(){

  bool status = init_wayland();
  
  pway_display = wayland_terminal.display;
  pway_surface = wayland_terminal.wayland_surface;

  pway = malloc(sizeof(PWay));
  return pway;  
}

bool pway_create_window(const char* name){

  xdg_toplevel_set_title(wayland_terminal.window, name);

}

void pway_finish(void){

  wl_display_disconnect(wayland_terminal.display);
}

