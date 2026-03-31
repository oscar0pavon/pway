#include "wayland.h"
#include "cursor_shape_protocol.h"
#include "keyboard.h"
#include "mouse.h"
#include "primary_selection.h"
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <pthread.h>
#include "input.h"
#include "data.h"

#include "pway.h"
#include "selection.h"

PWayland wayland = {};

typedef struct WaylandInitialization{
  bool compositor;
  bool desktop;
}WaylandInitialization;

WaylandInitialization initialization;

bool handle_wayland_initialization(){
  if(!initialization.compositor){
    printf("Can't load compositor\n");
    return false;
  }
  if(!initialization.desktop){
    printf("Can't load desktop\n");
    return false;
  }

  return true;
}
void configure_surface(void* data, DesktopSurface *surface, uint32_t serial){

  PWayland* terminal = (PWayland*)data;

  xdg_surface_ack_configure(surface, serial);

}

void configure_window(void* data, struct xdg_toplevel* window, int width, 
    int height, struct wl_array *state){

  PWayland* terminal = data;

  //printf("Compositor suggested size %i %i\n",width, height); 

  if(width == 0 && height == 0){
    return;
  }


  pway->resize(width,height);


}

void handle_exit(void *data, struct xdg_toplevel* window){
  printf("Request close window\n");
  pway->exit();
}

SurfaceListener surface_listener = {
  .configure = configure_surface
};

WindowListener window_listener = {
  .configure = configure_window,
  .close = handle_exit
};

bool is_registry_name(const char* name1, const char* name2){

  if (strcmp(name1, name2) == 0) {
    return true;
  }else {
    return false;
  }
}

void register_global(void *data, Registry *registry, uint32_t name_id,
    const char *interface_name, uint32_t version) {

  PWayland *way = (PWayland*)data;

  if (is_registry_name(interface_name, wl_compositor_interface.name) ) {

    initialization.compositor = true;
    way->compositor =
        wl_registry_bind(registry, name_id, &wl_compositor_interface, 4);

  } else if (is_registry_name(interface_name, xdg_wm_base_interface.name) ) {

    initialization.desktop = true;

    way->desktop =
        wl_registry_bind(registry, name_id, &xdg_wm_base_interface, 1);

  } else if (is_registry_name(interface_name, wl_shm_interface.name)) {

    wl_registry_bind(registry, name_id, &wl_shm_interface, 1);


  } else if (is_registry_name(interface_name, wl_seat_interface.name)) {

    way->seat = wl_registry_bind(registry, name_id, &wl_seat_interface, 4);

    configure_input(&wayland);

  } else if( is_registry_name(interface_name, wl_data_device_manager_interface.name)){
    way->data_device_manager = wl_registry_bind(
          registry,
          name_id,
          &wl_data_device_manager_interface,
          3
        );
  } else if ( is_registry_name(interface_name, "zwp_primary_selection_device_manager_v1") ) {

    primary_selection.primary_selection_manager = 
      wl_registry_bind(registry, name_id,
          &zwp_primary_selection_device_manager_v1_interface, 1);

    configure_selection();

  } else if ( is_registry_name(interface_name, wp_cursor_shape_manager_v1_interface.name)){

    way->cursor_shape_manager = wl_registry_bind(registry, name_id, 
        &wp_cursor_shape_manager_v1_interface, 1);

  }

}

void remove_global(void *data, Registry *registry, uint32_t name) {}

RegistryListener registry_listener = {
  .global = register_global,
  .global_remove = remove_global
};

void pway_prepare_to_read_events(){
  while(wl_display_prepare_read(wayland.display) != 0)
    wl_display_dispatch_pending(wayland.display);

  wl_display_flush(wayland.display);
}

void pway_handle_events_pressed(){
  clean_mouse_buttons();
}

void* run_wayland_loop(void*none){
  
  while(wl_display_dispatch(wayland.display)){
   //events 
  }

  return NULL;
}


bool init_wayland() {
  wayland.display = wl_display_connect(NULL);
  if(wayland.display == NULL){
    printf("Can't get Wayland display, trying Xorg\n");
    return false;
  }

  wayland.registry = wl_display_get_registry(wayland.display);

  wl_registry_add_listener(wayland.registry, &registry_listener, 
      &wayland);

  wl_display_roundtrip(wayland.display);


  bool init = handle_wayland_initialization();
  if(!init)
    return false;

  wayland.wayland_surface =
      wl_compositor_create_surface(wayland.compositor);

  wayland.desktop_surface = xdg_wm_base_get_xdg_surface(
      wayland.desktop, wayland.wayland_surface);

  xdg_surface_add_listener(wayland.desktop_surface, &surface_listener,
                           &wayland);

  wayland.window = xdg_surface_get_toplevel(wayland.desktop_surface);
  
  xdg_toplevel_add_listener(wayland.window, &window_listener, &wayland);


  wl_surface_commit(wayland.wayland_surface);
  wl_display_flush(wayland.display);


  pway->fd = wl_display_get_fd(wayland.display);
  init_keyboard_reapeat_handler();

  configure_data();

  printf("Waynland initialized\n");



  return true;
}
