#include "copy_paste.h"
#include "primary_selection.h"
#include "wayland.h"
#include <unistd.h>
#include <stdio.h>
#include <wayland-client-protocol.h>
#include "selection.h"
#include "pway.h"

void pway_update_wayland_paste_events(){

  while (wl_display_prepare_read(wayland.display) != 0) {
      wl_display_dispatch_pending(wayland.display);
  }
  wl_display_flush(wayland.display);
  wl_display_read_events(wayland.display);
  wl_display_dispatch_pending(wayland.display);
}

void pway_can_paste(){
  if( pway->fds[3].revents & POLLIN){
    printf("pasting event\n");
    char buffer[1024];
    ssize_t n = read(pway->paste_fds[0], buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        pway->input(buffer, n);
        printf("Received: %s\n", buffer);
    } else if (n == 0) {
        pway->fds[3].fd = -1; // Stop polling paste FD
        close(pway->paste_fds[0]);
    } 
  }
}

void pway_paste(bool is_primary){
  if(!is_primary){
    if(!wayland.active_data_offer){
      printf("None data to paste\n");
      return;
    }
  }else{
    if(!primary_selection.offer){
      printf("None data to paste from primary\n");
      return;
    }
  }

  if (pipe(pway->paste_fds) == -1) return; // Create a Unix pipe

  if(!is_primary){

    if(wayland.active_data_offer){

      wl_data_offer_receive(wayland.active_data_offer, 
          "text/plain", pway->paste_fds[1]);
    }

  }else{
    
    if(primary_selection.offer){
      zwp_primary_selection_offer_v1_receive(primary_selection.offer,
          "text/plain", pway->paste_fds[1]);
    }
  }

  wl_display_flush(wayland.display);

  close(pway->paste_fds[1]);


  pway_update_wayland_paste_events();
  pway->fds[3].fd = pway->paste_fds[0];

  
  printf("pway pasting\n");
}
