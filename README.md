# pway
Library for create wayland's window

## How to use - Example:  

```C
#include <pway/pway.h>

int main(){
  pway = pway_init();

  pway->resize = resize_pterminal;
  pway->exit = end_window;
  pway->focus = focus_window;
  pway->input = input_keys;
  pway->click = mouse_click;
  pway->click_release = release_button;
  pway->update_mouse = update_mouse;
  pway->get_text = get_selection;

}

```
