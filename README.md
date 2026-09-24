# WINLOCK #

X11 window control utility. Locks window position by default, with optional size lock, input blocking, and window manager property overrides. Stays alive to maintain position locks and input grabs; exits immediately when only one-shot properties are applied.

## dependencies ##

``` shell
sudo apt install libx11-dev
```

## building ##

``` shell
cd ${project_root}
make clean && make
```

Binary output: `build/winlock`

``` shell
sudo make install
```

## usage ##

``` shell
winlock <window_id> [options...]
```

| argument | description |
| :--- | :--- |
| `window_id` | X11 window ID in hex (`0x...`) or decimal |

## options ##

| option | description |
| :--- | :--- |
| `--resize-lock [WxH]` | lock window size (current size if WxH omitted) |
| `--position-lock=0` | disable default position locking |
| `--no-right-click` | block right-click (button 3) |
| `--no-middle-click` | block middle-click (button 2) |
| `--skip-taskbar` | hide from taskbar |
| `--skip-pager` | hide from pager/alt-tab |
| `--no-decorations` | remove title bar and borders |
| `--opacity <0.0-1.0>` | set window transparency |
| `--oneshot` | apply settings and exit (no event loop) |
| `-h`, `--help` | show help message |
| `-v`, `--version` | show version number |

## behavior ##

Position locking is enabled by default. When active, winlock monitors `ConfigureNotify` events and moves the window back to its initial position if it changes. Disable with `--position-lock=0`.

Features that require an event loop (position lock, button grabs) keep the process alive. One-shot properties (resize lock, EWMH states, decorations, opacity) are applied immediately. If only one-shot properties are used, the process exits after applying them. The `--oneshot` flag forces exit regardless.

The process handles `SIGTERM`/`SIGINT`/`SIGHUP` for clean shutdown and exits automatically if the target window is destroyed (`DestroyNotify` or `BadWindow` error).

## examples ##

``` shell
# lock a window's position (default behavior)
wid=$(wmctrl -l | awk '/MY_WINDOW/{print $1; exit}')
winlock "$wid"

# lock position and size, block right-click
winlock "$wid" --resize-lock --no-right-click

# lock to specific size, no decorations, skip taskbar
winlock "$wid" --resize-lock=640x456 --no-decorations --skip-taskbar

# one-shot: set opacity and exit
winlock "$wid" --opacity 0.85 --position-lock=0 --oneshot
```

## how it works ##

**Size lock**: sets `WM_NORMAL_HINTS` with `min_width == max_width` and `min_height == max_height` per the ICCCM spec. Window managers treat this as non-resizable - the resize cursor is removed from window edges. Existing size hints (base size, resize increments) are preserved.

**Position lock**: records the window's initial position and watches for `ConfigureNotify` events. If the window moves, it is immediately repositioned via `XMoveWindow`.

**Input blocking**: uses `XGrabButton` to capture and discard button events on the target window.

**EWMH states**: modifies `_NET_WM_STATE` atoms (`SKIP_TASKBAR`, `SKIP_PAGER`) via `XChangeProperty`.

**Decorations**: sets the Motif `_MOTIF_WM_HINTS` property with the `MWM_HINTS_DECORATIONS` flag and `decorations = 0` to remove title bar and borders.

**Opacity**: sets `_NET_WM_WINDOW_OPACITY` as a 32-bit cardinal (0 to 0xFFFFFFFF).

## file structure ##

``` shell
winlock/
├── src/
│   ├── main.c         # entry point, event loop, signal handling
│   ├── main.h         # WinlockConfig struct, constants
│   ├── config.c       # CLI argument parsing (getopt_long)
│   ├── xutil.c        # X11 utility functions (size, position, EWMH, grabs)
│   └── xutil.h        # xutil function declarations
├── build/
│   └── winlock        # compiled binary (after make)
├── Makefile
└── README.md
```
