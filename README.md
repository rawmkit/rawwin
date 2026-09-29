# rawwin

`rawwin` is the interactive managed-window selector for the rawm desktop.

It owns no window-manager state and speaks no private rawm protocol. Its job is
composition only:

1. obtain rawm's authoritative admitted-client inventory with `rawctl clients`;
2. present that inventory through `rawmenu` select mode;
3. submit the selected XID with `rawctl focus-client`.

This keeps the three authorities separate: rawm decides which clients are
admitted and which client becomes focused, rawmenu owns interactive selection
presentation, and rawwin owns only the user workflow joining those public
interfaces.

## Requirements

* a C99 compiler;
* Meson 1.6 or newer;
* `scdoc(1)` for the manual page;
* `rawctl` at run time;
* `rawmenu` at run time.

There is no X11 library dependency in rawwin itself.

## Build

```sh
meson setup build
meson compile -C build
meson test -C build --print-errorlogs
meson install -C build
```

## Usage

Run:

```sh
rawwin
```

The menu contains every client in rawm's authoritative client inventory. If a
client disappears while the menu is open, `rawctl focus-client` rejects the
stale selection rather than reconstructing or activating it through EWMH.

## Boundary

rawwin deliberately does not enumerate the X tree, inspect `_NET_CLIENT_LIST`,
send `_NET_ACTIVE_WINDOW`, parse rawm private structures, own keyboard grabs, or
implement selector presentation. It is a stateless leaf client composed from
rawctl and rawmenu.
