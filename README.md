# dedwm - Minimal Dynamic Window Manager

A minimal implementation of dwm (dynamic window manager) for X11.

## Description

dedwm is a stripped-down, minimal version of the dynamic window manager (dwm). It provides basic window management functionality with a focus on simplicity and small code size.

## Features

- Tiling, monocle, and floating layouts
- Multiple workspaces (tags)
- Keyboard-driven workflow
- Minimal dependencies
- Small executable size

## Requirements

In order to build dedwm you need the Xlib header files.

## Installation

Edit config.mk to match your local setup (dwm is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install dwm (if
necessary as root):

```bash
make clean install
```

## Running dedwm

Add the following line to your .xinitrc to start dwm using startx:

```bash
exec dwm
```

In order to connect dwm to a specific display, make sure that
the DISPLAY environment variable is set correctly, e.g.:

```bash
DISPLAY=foo.bar:1 exec dwm
```

## Configuration

The configuration of dwm is done by editing config.h and (re)compiling the
source code.

## Default Keybindings

- `Mod1+Return`: spawn terminal (st)
- `Mod1+Shift+Return`: zoom (swap with master)
- `Mod1+j/k`: focus next/previous window
- `Mod1+h/l`: decrease/increase master area
- `Mod1+Shift+c`: kill focused window
- `Mod1+[1-9]`: view tag
- `Mod1+Shift+[1-9]`: send window to tag
- `Mod1+t/f/m`: tile/floating/monocle layout
- `Mod1+Shift+q`: quit dwm

(Mod1 is typically Alt key)

## License

See LICENSE file for details.
