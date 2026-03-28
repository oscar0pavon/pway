#include "wayland.h"
#include "keyboard.h"
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

PWayland wayland_terminal = {};

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

void register_global(void *data, Registry *registry, uint32_t name_id,
    const char *interface_name, uint32_t version) {

  PWayland *terminal = (PWayland*)data;

  if (strcmp(interface_name, wl_compositor_interface.name) == 0) {

    initialization.compositor = true;
    terminal->compositor =
        wl_registry_bind(registry, name_id, &wl_compositor_interface, 4);

  } else if (strcmp(interface_name, xdg_wm_base_interface.name) == 0) {

    initialization.desktop = true;

    terminal->desktop =
        wl_registry_bind(registry, name_id, &xdg_wm_base_interface, 1);

  } else if (strcmp(interface_name, wl_shm_interface.name) == 0) {

    wl_registry_bind(registry, name_id, &wl_shm_interface, 1);


  } else if (strcmp(interface_name, wl_seat_interface.name) == 0) {

    terminal->seat = wl_registry_bind(registry, name_id, &wl_seat_interface, 4);

    configure_input(&wayland_terminal);

  } else if( strcmp(interface_name, wl_data_device_manager_interface.name) == 0){
    terminal->data_device_manager = wl_registry_bind(
          registry,
          name_id,
          &wl_data_device_manager_interface,
          3
        );
  } else if (strcmp(interface_name, "zwp_primary_selection_device_manager_v1") == 0) {

    primary_selection.primary_selection_manager = 
      wl_registry_bind(registry, name_id,
          &zwp_primary_selection_device_manager_v1_interface, 1);

    printf("Primary selection global detected\n");
    configure_selection();
  }

}

void remove_global(void *data, Registry *registry, uint32_t name) {}

RegistryListener registry_listener = {
  .global = register_global,
  .global_remove = remove_global
};

void pway_prepare_to_read_events(){
  while(wl_display_prepare_read(wayland_terminal.display) != 0)
    wl_display_dispatch_pending(wayland_terminal.display);

  wl_display_flush(wayland_terminal.display);
}

void* run_wayland_loop(void*none){
  
  while(wl_display_dispatch(wayland_terminal.display)){
   //events 
  }

  return NULL;
}

void pway_handle_events(){

  pway_prepare_to_read_events();

  if (poll(pway->fds, 3, -1) == -1) {
    perror("Poll in fds, APP or Wayland, Keyboard timer");
  }

  if(pway->fds[0].revents & POLLIN){
    wl_display_read_events(wayland_terminal.display);
    //printf("wayland poll\n");
  }else{
    wl_display_cancel_read(wayland_terminal.display);
  }

  wl_display_dispatch_pending(wayland_terminal.display);

  handle_repeat_keys();

}

bool init_wayland() {
  wayland_terminal.display = wl_display_connect(NULL);
  if(wayland_terminal.display == NULL){
    printf("Can't get Wayland display, trying Xorg\n");
    return false;
  }

  wayland_terminal.registry = wl_display_get_registry(wayland_terminal.display);

  wl_registry_add_listener(wayland_terminal.registry, &registry_listener, 
      &wayland_terminal);

  wl_display_roundtrip(wayland_terminal.display);


  bool init = handle_wayland_initialization();
  if(!init)
    return false;

  wayland_terminal.wayland_surface =
      wl_compositor_create_surface(wayland_terminal.compositor);

  wayland_terminal.desktop_surface = xdg_wm_base_get_xdg_surface(
      wayland_terminal.desktop, wayland_terminal.wayland_surface);

  xdg_surface_add_listener(wayland_terminal.desktop_surface, &surface_listener,
                           &wayland_terminal);

  wayland_terminal.window = xdg_surface_get_toplevel(wayland_terminal.desktop_surface);
  
  xdg_toplevel_add_listener(wayland_terminal.window, &window_listener, &wayland_terminal);


  //wl_display_roundtrip(wayland_terminal.display);
  wl_surface_commit(wayland_terminal.wayland_surface);//TODO
  wl_display_flush(wayland_terminal.display);


  pway->fd = wl_display_get_fd(wayland_terminal.display);
  init_keyboard_reapeat_handler();

  configure_data();

  printf("Waynland initialized\n");



  return true;
}
