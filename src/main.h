/*
 * main.h - winlock types and constants
 */

#ifndef WINLOCK_MAIN_H
#define WINLOCK_MAIN_H

#include <signal.h>
#include <X11/Xlib.h>

#ifndef VERSION
#define VERSION "dev"
#endif

typedef struct {
    Window wid;

    /* position lock (default: on) */
    int position_lock;

    /* resize lock (opt-in) */
    int resize_lock;
    int width;          /* 0 = use current */
    int height;

    /* input blocking */
    int no_right_click;
    int no_middle_click;

    /* EWMH states */
    int skip_taskbar;
    int skip_pager;

    /* decorations */
    int no_decorations;

    /* opacity: 0.0 = don't set, 0.01-1.0 = translucent to opaque */
    double opacity;

    /* behavior */
    int oneshot;        /* apply and exit, skip event loop */
} WinlockConfig;

/* signal flag for clean shutdown */
extern volatile sig_atomic_t winlock_exit;

#endif /* WINLOCK_MAIN_H */
