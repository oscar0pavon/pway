# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`pway` is a small C static library (`libpway.a`) that gives an application a Wayland
window plus keyboard, mouse, EGL context, and clipboard/primary-selection handling.
It is a library only — there is no executable, no test suite, and no linting setup here.
It was extracted from a terminal emulator (`pterminal`), which is still visible in
leftover comments and in the `# pterminal` header of the Makefile.

## Build

```sh
make            # -> libpway.a  (implicit .c.o rule, FLAGS = -g)
make clean
sudo make install   # libpway.a -> /usr/local/lib ; public headers -> /usr/local/include/pway
```

`make -j24` is safe. Note the Makefile is `.POSIX:` with `SRC = $(wildcard *.c)`, so any
new `.c` file in the root is picked up automatically — including generated protocol code.

Compile-time deps: `wayland-client`, `wayland-egl`, `EGL`, `xkbcommon`, Linux
`timerfd`/`poll`. Consumers must link those themselves:
`-lpway -lwayland-client -lwayland-egl -lEGL -lxkbcommon`.

Only `pway.h`, `mouse.h`, `keyboard.h`, `copy_paste.h` are installed — that set is the
public API surface. If you add a public type, install its header too.

## Generated protocol code — do not hand-edit

`xdg_protocol.[ch]`, `primary_selection.[ch]`, and `cursor_shape_protocol.[ch]` are
`wayland-scanner` output and are committed to the repo. Regenerate with:

```sh
./generate_wayland_client_files.sh   # needs wayland-scanner + /usr/share/wayland-protocols
```

Any edit to them will be lost on the next regeneration. To add a protocol, add the
`wayland-scanner client-header` / `private-code` pair to that script and re-run it.

## Architecture

Two file-scope globals carry all state; nearly every function reaches them directly
instead of taking parameters:

- `pway` (`PWay*`, `pway.h`) — the public handle: app callbacks, `fds[4]`, mouse state,
  window size. Also the *input* channel for app config.
- `wayland` (`PWayland`, `wayland.h`) — internal registry/seat/surface/manager objects.

Naming convention: public functions are `pway_*`, public types are `P`-prefixed
(`PWay`, `PMouse`, `PKey`, `PPressEvent`).

### Callbacks are the whole API

The app fills in function pointers on `pway` (see README for the list). The library calls
them **unconditionally, without NULL checks at the call sites** — safety comes from
`pway_init` allocating with `calloc` and then installing a no-op default for every
callback via `set_default_callbacks()`, *before* `init_wayland()` runs (the compositor can
already dispatch events during that roundtrip). Two rules follow: when you add a callback
give it a default in the same place, and never move `set_default_callbacks()` after
`init_wayland()`.

### Startup order matters

`pway_init()` → `pway_create_window(name, w, h)` → `pway_init_egl()`.
`init_wayland()` (called from `pway_init`) already creates the `wl_surface` and
xdg toplevel; `pway_create_window` only sets the title and records `width`/`height`.
`init_egl()` then reads those dimensions to size the `wl_egl_window`, so calling it
before `pway_create_window` yields a 0×0 surface. `init_wayland`'s return value is
currently discarded in `pway_init`.

### Event loop: 4 poll fds, fixed slots

`pway_handle_events()` is one iteration of the app's main loop. `pway->fds` slots are
positional and referenced by index across files — keep them in sync if you touch them:

| idx | fd | consumed by |
|---|---|---|
| 0 | Wayland display fd | `pway_handle_events` (`wl_display_read_events`) |
| 1 | key-repeat `timerfd` | `handle_repeat_keys` (`keyboard.c`) |
| 2 | app-supplied fd, set via `pway_set_app_fd` / tested via `pway_app_has_event` | the app |
| 3 | read end of the paste pipe | `pway_can_paste` (`copy_paste.c`) |

The loop follows the mandatory Wayland prepare-read protocol:
`pway_prepare_to_read_events()` (prepare + flush) → `poll` → `read_events` on POLLIN or
`wl_display_cancel_read` otherwise → `dispatch_pending`. Never add a `poll` on the
display fd outside this dance, or events will be dropped/deadlocked.
`clean_mouse_buttons()` runs at the end of each iteration and clears a
press/release pair only on the *second* iteration after release, so a click stays
visible to the app for one full frame.

### Clipboard: two parallel stacks

Both go through `pway->get_text()` (app supplies the text to send) and
`pway->input()` (pasted text is delivered as if typed).

- Regular clipboard (`wl_data_device`): offers tracked in `data.c`
  (`wayland.active_data_offer`), copy in `data_copy.c` (`perform_copy`).
- Primary selection (`zwp_primary_selection_v1`): offers in `selection.c`
  (`primary_selection.offer`), copy in `data_copy.c` (`pway_primary_copy`).

**Neither manager is guaranteed to exist.** They are bound from the registry, so a
compositor that does not advertise them (swordfish does not) leaves
`wayland.data_device_manager` and `primary_selection.primary_selection_manager` NULL
and `configure_data()` / `configure_selection()` never build a device. Marshalling a
request on a NULL proxy is a **segfault inside libwayland-client** — no protocol error,
no message, the app just dies — so every entry point has to check first.
`pway_primary_copy()` did not, and finishing a mouse selection under swordfish killed
the client on the button release.

`pway_paste()` creates a pipe, asks the offer to write into it, then parks the read end
in `fds[3]` so the actual text arrives on a later loop iteration — pasting is
asynchronous, never inline.

Note `handle_key_sym` in `keyboard.c` intercepts **Ctrl+Shift+C/V** inside the library
and never forwards them to `pway->input`. Keyboard input is otherwise translated with
`xkb_keysym_to_utf8`; the non-Ctrl path passes `len - 1` to drop the NUL, the Ctrl path
passes `len`.

### Input details

`input.c` handles `wl_seat` capabilities and lazily calls `configure_mouse()` /
`configure_keyboard()`. Key codes coming from Wayland need `+ 8` before xkb lookup
(`keyboard.c`). Serials from the last press are cached
(`main_keyboard.last_input_serial`, `pway->mouse.last_input_serial`) because clipboard
and cursor-shape requests require a valid input serial.

## Conventions in this codebase

- 2-space indent, `{` on the same line, generous blank lines; generated protocol files
  keep wayland-scanner's own style.
- Diagnostics go to stdout via `printf` — there is no logging abstraction, and several
  of these are debug prints left in the event path (`"pasting event"`, `"Clean mouse
  buttons"`).
- Errors are reported and execution continues (see `init_egl`); the library largely does
  not propagate failure to the caller.
- `*.o` and `*.a` are gitignored — do not commit build output.
