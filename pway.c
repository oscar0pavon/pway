#include "pway.h"
#include "wayland.h"
#include <stdbool.h>


bool pway_create_window(const char* name){
 
  bool status = init_wayland();

  if(status == true)
    xdg_toplevel_set_title(wayland_terminal.window, name);

  pway_display = wayland_terminal.display;

  return status;
}

void pway_finish(void){

  wl_display_disconnect(wayland_terminal.display);
}

