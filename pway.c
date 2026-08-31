#include "pway.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <error.h>
#include "egl.h"
#include "shm.h"
#include "wayland.h"
#include "keyboard.h"
#include "copy_paste.h"

PWay* pway;

struct wl_display *pway_display;
struct wl_surface *pway_surface;



void pway_init_egl(){
  init_egl();
}

void pway_init_shm(){
  init_shm();
}

void pway_shm_resize(int width, int height){
  shm_resize(width, height);
}

uint32_t* pway_shm_get_buffer(int *stride){
  return shm_get_buffer(stride);
}

void pway_shm_commit(int x, int y, int width, int height){
  shm_commit(x, y, width, height);
}

bool pway_app_has_event(){
  
  return pway->fds[2].revents & POLLIN;

}

void pway_set_app_fd(int fd){
  pway->fds[2].fd = fd;
}


static void default_resize(int x, int y){}
static void default_exit(void){}
static void default_focus(bool is_focused){}
static void default_input(const char* text, int len){}
static void default_key(uint32_t key_code, uint32_t state){}
static void default_update_keys(void){}
static void default_click(void){}
static void default_click_release(void){}
static void default_update_mouse(void){}

static char no_text[] = "";

static char* default_get_text(void){
  return no_text;
}

// Every callback is called unconditionally from the event handling, and the
// compositor can send events during init_wayland, so these have to be in place
// before anything else runs. Leaving one unset is fine, setting one to NULL is
// not.
static void set_default_callbacks(){

  pway->resize = default_resize;
  pway->exit = default_exit;
  pway->focus = default_focus;
  pway->input = default_input;
  pway->key = default_key;
  pway->update_keys = default_update_keys;
  pway->click = default_click;
  pway->click_release = default_click_release;
  pway->update_mouse = default_update_mouse;
  pway->get_text = default_get_text;

}

PWay* pway_init(){

  pway = calloc(1, sizeof(PWay));
  if(pway == NULL){
    printf("Can't allocate PWay\n");
    return NULL;
  }

  set_default_callbacks();

  bool status = init_wayland();
  if(!status)
    return NULL;
  
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


static bool connection_lost = false;

bool pway_connection_lost(void){
  return connection_lost;
}

//A compositor that goes away does not stop this loop on its own. The display fd
//stays readable forever with POLLHUP, and every libwayland call made after the
//connection breaks returns -1 without touching the socket - so nothing blocks,
//nothing fails loudly, and poll() spins on a full core until the process is
//killed. Report it once and hand the app its exit callback.
static void lose_connection(void){

  if(connection_lost)
    return;

  connection_lost = true;

  int error = wl_display_get_error(wayland.display);

  printf("Lost the Wayland connection: %s\n",
      error ? strerror(error) : "hang up");

  pway->exit();

}

//the second half of pway_handle_events(), split out so an embedder that polls
//pway->fds itself (alongside fds of its own) can skip pway's internal poll()
//and just tell it the read side is ready. pway_prepare_to_read_events() must
//still be called before that external poll - this only takes over from the
//poll onward
void pway_dispatch_events(){

  if(pway->fds[0].revents & POLLIN){
    wl_display_read_events(wayland.display);
    //printf("wayland poll\n");
  }else{
    wl_display_cancel_read(wayland.display);
  }

  wl_display_dispatch_pending(wayland.display);

  //POLLHUP on its own never reaches read_events, so the poll flags have to be
  //checked as well as the connection error - either one means the display is
  //gone and none of the handlers below have anything left to work with.
  if(pway->fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)
      || wl_display_get_error(wayland.display) != 0){
    lose_connection();
    return;
  }

  handle_repeat_keys();


  pway_can_paste();

  clean_mouse_buttons();

}

void pway_handle_events(){

  if(connection_lost)
    return;

  if(!pway_prepare_to_read_events()){
    lose_connection();
    return;
  }

  if (poll(pway->fds, 4, -1) == -1) {
    perror("Poll in fds, APP or Wayland, Keyboard timer");
  }

  pway_dispatch_events();

}

bool pway_create_window(const char* name, int width, int height){

  xdg_toplevel_set_title(wayland.window, name);
  pway->width = width;
  pway->height = height;

  return true;

}
void pway_finish(void){

  wl_display_disconnect(wayland.display);
}

