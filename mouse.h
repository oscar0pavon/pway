#ifndef WMOUSE_H
#define WMOUSE_H

#include <stdint.h>

#define Button1     1
#define Button2     2
#define Button3     3
#define Button4     4
#define Button5     5

typedef struct PMouseButton{
  uint32_t id;
  bool pressed;
  bool released;
}PMouseButton;

typedef struct PMouse {
  uint32_t x;
  uint32_t y;

  PMouseButton* current_button;

  PMouseButton left_button;
  PMouseButton right_button;
  PMouseButton middle_button;

  PMouseButton wheel_up;
  PMouseButton wheel_down;
  
  uint32_t last_input_serial;

}PMouse;


void configure_mouse(void);
void release_mouse_button();

void clean_mouse_buttons();

extern PMouse main_mouse;

#endif
