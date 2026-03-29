#ifndef WMOUSE_H
#define WMOUSE_H

#include <stdint.h>
#include "copy_paste.h"

#define Button1     1
#define Button2     2
#define Button3     3
#define Button4     4
#define Button5     5

typedef struct PPressEvent{
  uint32_t id;
  bool pressed;
  bool released;
}PPressEvent;

typedef struct PMouse {
  uint32_t x;
  uint32_t y;

  PPressEvent* current_button;

  PPressEvent left_button;
  PPressEvent right_button;
  PPressEvent middle_button;

  PPressEvent wheel_up;
  PPressEvent wheel_down;
  
  uint32_t last_input_serial;

}PMouse;


void configure_mouse(void);
void release_mouse_button();

void clean_mouse_buttons();

void init_cursor_shape_protocol();

void pway_set_text_cursor();

void pway_set_default_cursor();

#endif
