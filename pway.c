#include "pway.h"
#include "wayland.h"
#include <stdbool.h>


bool pway_create_window(const char* name){
 
  bool status = init_wayland();

  if(status == true)
    xdg_toplevel_set_title(wayland_terminal.window, name);

  return status;
}

