/*
 * main.c - program entry point
 *
 * X11 window control: position/size lock, input grabs, WM state changes.
 */

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "main.h"
#include "xutil.h"

/* config.c */
extern void config_init(WinlockConfig *cfg);
extern int  config_parse(WinlockConfig *cfg, int argc, char **argv);

volatile sig_atomic_t winlock_exit = 0;
static int signal_pipe[2] = { -1, -1 };

static void handle_signal(int sig) {
    (void)sig;
    winlock_exit = 1;
    /* Wake select(); write() is async-signal-safe */
    if (signal_pipe[1] >= 0) {
        const char b = 1;
        ssize_t r = write(signal_pipe[1], &b, 1);
        (void)r;
    }
}

static int x_error_handler(Display *dpy, XErrorEvent *ev) {
    (void)dpy;
    if (ev->error_code == BadWindow) {
        winlock_exit = 1;
        return 0;
    }
    /* Default: print and continue */
    char buf[256];
    XGetErrorText(dpy, ev->error_code, buf, sizeof(buf));
    fprintf(stderr, "winlock: X error: %s\n", buf);
    return 0;
}

/* Xlib's default IO error handler calls _exit(1) on connection loss;
 * return cleanly so our normal cleanup path runs instead. */
static int x_io_error_handler(Display *dpy) {
    (void)dpy;
    winlock_exit = 1;
    return 0;
}

int main(int argc, char *argv[]) {
    WinlockConfig cfg;
    config_init(&cfg);

    if (config_parse(&cfg, argc, argv) != 0)
        return EXIT_FAILURE;

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "winlock: cannot open display\n");
        return EXIT_FAILURE;
    }

    XSetErrorHandler(x_error_handler);
    XSetIOErrorHandler(x_io_error_handler);

    /* Validate window exists */
    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, cfg.wid, &attr)) {
        fprintf(stderr, "winlock: invalid window 0x%lx\n", cfg.wid);
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }

    /* Apply one-shot properties */
    if (cfg.resize_lock)
        xutil_lock_size(dpy, cfg.wid, cfg.width, cfg.height);

    if (cfg.skip_taskbar)
        xutil_set_state(dpy, cfg.wid, "_NET_WM_STATE_SKIP_TASKBAR", 1);

    if (cfg.skip_pager)
        xutil_set_state(dpy, cfg.wid, "_NET_WM_STATE_SKIP_PAGER", 1);

    if (cfg.no_decorations)
        xutil_set_decorations(dpy, cfg.wid, 0);

    if (cfg.opacity > 0.0)
        xutil_set_opacity(dpy, cfg.wid, cfg.opacity);

    /* Button grabs on client window and WM frame */
    Window frame = xutil_get_frame(dpy, cfg.wid);

    if (cfg.no_right_click) {
        xutil_grab_button(dpy, cfg.wid, Button3);
        if (frame != cfg.wid)
            xutil_grab_button(dpy, frame, Button3);
    }

    if (cfg.no_middle_click) {
        xutil_grab_button(dpy, cfg.wid, Button2);
        if (frame != cfg.wid)
            xutil_grab_button(dpy, frame, Button2);
    }

    /* Record initial position for position lock */
    int lock_x = 0, lock_y = 0;
    if (cfg.position_lock)
        xutil_get_position(dpy, cfg.wid, &lock_x, &lock_y);

    /* Determine whether to stay alive */
    int need_loop = cfg.position_lock || cfg.no_right_click || cfg.no_middle_click;

    if (!need_loop || cfg.oneshot) {
        XFlush(dpy);
        XCloseDisplay(dpy);
        return EXIT_SUCCESS;
    }

    /* Self-pipe so signals can wake select() */
    if (pipe(signal_pipe) != 0) {
        fprintf(stderr, "winlock: pipe() failed\n");
        XCloseDisplay(dpy);
        return EXIT_FAILURE;
    }
    for (int i = 0; i < 2; i++) {
        int fl = fcntl(signal_pipe[i], F_GETFL, 0);
        fcntl(signal_pipe[i], F_SETFL, fl | O_NONBLOCK);
        int fd = fcntl(signal_pipe[i], F_GETFD, 0);
        fcntl(signal_pipe[i], F_SETFD, fd | FD_CLOEXEC);
    }

    /* Event loop: maintain position lock and button grabs */
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    /* Select events on target window (and frame if different) */
    long mask = StructureNotifyMask;
    XSelectInput(dpy, cfg.wid, mask);
    if (frame != cfg.wid)
        XSelectInput(dpy, frame, StructureNotifyMask);
    XFlush(dpy);

    int x_fd = ConnectionNumber(dpy);

    while (!winlock_exit) {
        /* Drain queued events before blocking in select() */
        while (!winlock_exit && XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
            case ConfigureNotify:
                if (cfg.position_lock && ev.xconfigure.window == cfg.wid) {
                    int cur_x, cur_y;
                    xutil_get_position(dpy, cfg.wid, &cur_x, &cur_y);
                    if (cur_x != lock_x || cur_y != lock_y) {
                        XMoveWindow(dpy, cfg.wid, lock_x, lock_y);
                        XFlush(dpy);
                    }
                }
                break;

            case ButtonPress:
                /* Grabbed button: discard */
                break;

            case DestroyNotify:
                winlock_exit = 1;
                break;
            }
        }

        if (winlock_exit)
            break;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(x_fd, &rfds);
        FD_SET(signal_pipe[0], &rfds);
        int maxfd = x_fd > signal_pipe[0] ? x_fd : signal_pipe[0];

        int r = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (r < 0) {
            if (errno == EINTR) continue;
            /* Unexpected: bail rather than spin */
            break;
        }

        if (FD_ISSET(signal_pipe[0], &rfds)) {
            /* Drain pipe; exit flag already set by handler */
            char buf[16];
            while (read(signal_pipe[0], buf, sizeof(buf)) > 0) { }
            break;
        }
    }

    if (signal_pipe[0] >= 0) close(signal_pipe[0]);
    if (signal_pipe[1] >= 0) close(signal_pipe[1]);
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
