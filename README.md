# pway

Library for create wayland's window.

`pway` gives a C application a Wayland window with keyboard, mouse, an EGL/OpenGL
context, and clipboard + primary selection support. It builds as a static library,
`libpway.a`.

## Dependencies

- `wayland-client`, `wayland-egl`, `wayland-protocols` (plus `wayland-scanner` to
  regenerate the protocol code)
- `EGL` and a desktop OpenGL driver
- `libxkbcommon`
- Linux `timerfd` / `poll`

## Build

```sh
make
sudo make install
```

`make install` puts `libpway.a` in `/usr/local/lib` and the public headers in
`/usr/local/include/pway`. Link your application with:

```sh
-lpway -lwayland-client -lwayland-egl -lEGL -lxkbcommon
```

The Wayland protocol sources (`xdg_protocol`, `primary_selection`,
`cursor_shape_protocol`) are generated and committed. Regenerate them with
`./generate_wayland_client_files.sh`; do not edit them by hand.

## How to use - Example:

```C
#include <pway/pway.h>

int main(){
  pway = pway_init();

  pway->resize = resize_pterminal;
  pway->exit = end_window;
  pway->focus = focus_window;
  pway->input = input_keys;
  pway->update_keys = update_keys;
  pway->click = mouse_click;
  pway->click_release = release_button;
  pway->update_mouse = update_mouse;
  pway->get_text = get_selection;

  pway_create_window("my window", 800, 600);
  pway_init_egl();

  while(is_running){
    pway_handle_events();

    draw();

    pway_swap_buffers();
  }

  pway_finish();
}
```

The order matters: `pway_init()` connects to the compositor and creates the surface,
`pway_create_window()` sets the title and the size, and `pway_init_egl()` uses that size
to create the EGL window. Calling `pway_init_egl()` first gives a 0x0 surface.

All the callbacks above are called without checking for `NULL`, and `pway_init()` does
not zero the struct, so assign every one of them before entering the loop.

Handle resizing by forwarding the new size to EGL:

```C
void resize_pterminal(int width, int height){
  pway->width = width;
  pway->height = height;
  pway_egl_resize(width, height);
}
```

## Callbacks

| Callback | Called when |
| --- | --- |
| `void resize(int width, int height)` | the compositor configures the toplevel with a non-zero size |
| `void exit(void)` | the compositor asks to close the window |
| `void focus(bool is_focused)` | the keyboard enters or leaves the surface |
| `void input(const char* text, int len)` | a key produced text, or pasted text arrived |
| `void update_keys(void)` | a key produced no text (arrows, function keys, modifiers) |
| `void click(void)` | a mouse button was pressed |
| `void click_release(void)` | a mouse button was released, or the wheel was scrolled |
| `void update_mouse(void)` | the pointer moved |
| `char* get_text(void)` | the compositor requests the text to copy; return the current selection |

`len` is the byte count of `text`, which the library NUL-terminates as well (with Ctrl
held and a non-letter key, `len` currently counts that trailing NUL too). Pasted text
arrives asynchronously, one or more loop iterations after the paste was requested.

## Window and rendering

```C
PWay* pway_init(void);
bool  pway_create_window(const char* name, int width, int height);
void  pway_handle_events(void);
void  pway_finish(void);

void  pway_init_egl(void);
void  pway_egl_resize(int width, int height);
void  pway_swap_buffers(void);
```

`pway_handle_events()` is one iteration of the main loop: it polls the Wayland
connection, the key repeat timer, the application fd and the paste pipe, dispatches
Wayland events, handles key repetition and finishes pending mouse button events. It
blocks until something happens, so call it once per frame.

The EGL context is a desktop OpenGL one (`EGL_OPENGL_BIT`, 8 bits per color channel, no
depth or alpha requested). `pway_display` and `pway_surface` are exported if you need the
raw Wayland objects.

## Your own file descriptors

```C
void pway_set_app_fd(int fd);
bool pway_app_has_event(void);
```

Register one application fd — a PTY, a socket — and `pway_handle_events()` will poll it
along with the Wayland connection instead of waking up on a timer:

```C
pway_set_app_fd(pty_fd);

while(is_running){
  pway_handle_events();

  if(pway_app_has_event())
    read_from_pty();

  draw();
  pway_swap_buffers();
}
```

## Keyboard

```C
extern PKey pway_current_key;   /* .sym is an xkb_keysym_t, .event has pressed/released */
```

Key repetition follows the compositor's rate and delay. `Ctrl+Shift+C` and
`Ctrl+Shift+V` are handled inside the library as copy and paste and are never forwarded
to `input`. Control combinations are translated to their control character (`Ctrl+A` to
`0x01`, and so on).

## Mouse

```C
typedef struct PPressEvent{
  uint32_t id;
  bool pressed;
  bool released;
}PPressEvent;
```

`pway->mouse` holds the pointer position in surface coordinates (`x`, `y`), one
`PPressEvent` per button (`left_button`, `right_button`, `middle_button`) and one for
each wheel direction (`wheel_up`, `wheel_down`). `pway->mouse.current_button` points at
the button of the event being reported, so `click` and `click_release` can tell which
one it was. A press/release pair stays visible for a full loop iteration before it is
cleared. Wheel scrolling is reported through `click_release`.

The cursor shape can be changed with the `cursor-shape-v1` protocol:

```C
void pway_set_text_cursor(void);
void pway_set_default_cursor(void);
```

## Clipboard

```C
void pway_paste(bool is_primary);
void pway_primary_copy(void);
```

`pway_paste(false)` reads the regular clipboard, `pway_paste(true)` the primary
selection; in both cases the text is later handed to the `input` callback.
`pway_primary_copy()` offers the current selection as the primary selection, taking the
text from `get_text`. Copying to the regular clipboard is done by `Ctrl+Shift+C`.

## License

GPLv3. See the LICENSE file.
