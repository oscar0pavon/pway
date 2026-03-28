#!/bin/sh

wayland-scanner client-header \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg_protocol.h
wayland-scanner private-code \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg_protocol.c


wayland-scanner client-header \
  /usr/share/wayland-protocols/unstable/primary-selection/primary-selection-unstable-v1.xml \
  primary_selection.h
wayland-scanner private-code \
  /usr/share/wayland-protocols/unstable/primary-selection/primary-selection-unstable-v1.xml \
  primary_selection.c


wayland-scanner client-header \
  /usr/share/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml cursor_shape_protocol.h
wayland-scanner private-code \
  /usr/share/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml cursor_shape_protocol.c
