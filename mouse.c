#include "mouse.h"

#include <linux/input.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "cursor_shape_protocol.h"
#include "pway.h"
#include "wayland.h"

void pway_set_text_cursor(){
  wp_cursor_shape_device_v1_set_shape(wayland.cursor_shape_device, pway->mouse.last_input_serial, 
      WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT);
}

void pway_set_default_cursor(){
  wp_cursor_shape_device_v1_set_shape(wayland.cursor_shape_device, pway->mouse.last_input_serial, 
      WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
}

void init_cursor_shape_protocol(){
  wayland.cursor_shape_device =
    wp_cursor_shape_manager_v1_get_pointer(wayland.cursor_shape_manager,
        wayland.pointer);

}

static void pointer_handle_enter(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface,
                                 wl_fixed_t sx, wl_fixed_t sy) {
    PWayland* term = data;
    wp_cursor_shape_device_v1_set_shape(wayland.cursor_shape_device,
        serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT);
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer,
                                  uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
  PWayland *term = data;
 
  pway->mouse.x = wl_fixed_to_double(sx);
  pway->mouse.y = wl_fixed_to_double(sy);

  pway->update_mouse();
 

}

void press_button(){

  if(pway->mouse.current_button){
    pway->mouse.current_button->pressed = true;
    pway->mouse.current_button->released = false;
  }

  pway->click();

}

void clean_mouse_buttons(){

  if(pway->mouse.current_button){
    if(pway->mouse.current_button->released){
      pway->mouse.current_button->released = false;
      pway->mouse.current_button = NULL;
    }
  }
}

void release_mouse_button(){

  if(pway->mouse.current_button){
    
    pway->mouse.current_button->pressed = false;
    pway->mouse.current_button->released = true;

  }

  pway->click_release();

}

static void pointer_handle_button(void *data, struct wl_pointer *pointer,
                                  uint32_t serial, uint32_t time, 
                                  uint32_t button, uint32_t state) {

  pway->mouse.current_button = NULL;

  switch (button) {
      case BTN_LEFT:
                   pway->mouse.current_button = &pway->mouse.left_button; 
                   pway->mouse.current_button->id = 0;
                   break;
      case BTN_MIDDLE:  
                   pway->mouse.current_button = &pway->mouse.middle_button; 
                   pway->mouse.current_button->id = 1;
                   break;
      case BTN_RIGHT:
                   pway->mouse.current_button = &pway->mouse.right_button; 
                   pway->mouse.current_button->id = 2;
                   break;
      default:
                   pway->mouse.current_button = NULL;
  }

  if (state == WL_POINTER_BUTTON_STATE_PRESSED) {

    pway->mouse.last_input_serial = serial;


    press_button();


  }else{

    release_mouse_button();


    
  }
}

static void pointer_handle_axis(void *data, struct wl_pointer *pointer,
                                uint32_t time, uint32_t axis, wl_fixed_t value) {

  double scroll_delta = wl_fixed_to_double(value);

  if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {

    if(value < 0){
      
        pway->mouse.current_button = &pway->mouse.wheel_down;
        release_mouse_button();

      // if(!is_on_mouse_mode()){
      //  Arg new_arg;
      //  new_arg.f = -0.1f;
      //  kscrollup(&new_arg);
      // }
    }else if(value > 0){

      // if(!is_on_mouse_mode()){
      //   Arg new_arg;
      //   new_arg.f = -0.1f;
      //   kscrolldown(&new_arg);
      // }else{
        pway->mouse.current_button = &pway->mouse.wheel_up;
        release_mouse_button();
      //
      // }
    }

  }

}

static void pointer_handle_frame(void *data, struct wl_pointer *pointer) {
    // Grouped events complete; trigger redraw if needed
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer,
                                 uint32_t serial, struct wl_surface *surface) {}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_handle_enter,
    .leave = pointer_handle_leave,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = pointer_handle_axis,
    .frame = pointer_handle_frame,
};

void configure_mouse(void){

  wl_pointer_add_listener(wayland.pointer, &pointer_listener, &wayland);

  pway->mouse.wheel_up.id = 65;
  pway->mouse.wheel_down.id = 64;
  
  init_cursor_shape_protocol();
  
}
