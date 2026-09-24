/*
 * xutil.c - X11 utility functions for winlock
 */

#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "xutil.h"

/* _MOTIF_WM_HINTS structure */
typedef struct {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints;

#define MWM_HINTS_DECORATIONS (1L << 1)

int xutil_lock_size(Display *dpy, Window wid, int w, int h) {
    if (w <= 0 || h <= 0) {
        XWindowAttributes attr;
        if (!XGetWindowAttributes(dpy, wid, &attr)) {
            fprintf(stderr, "winlock: cannot get window attributes for size lock\n");
            return -1;
        }
        w = attr.width;
        h = attr.height;
    }

    XSizeHints *hints = XAllocSizeHints();
    if (!hints) return -1;

    long supplied;
    XGetWMNormalHints(dpy, wid, hints, &supplied);

    hints->flags |= PMinSize | PMaxSize;
    hints->min_width  = hints->max_width  = w;
    hints->min_height = hints->max_height = h;

    XSetWMNormalHints(dpy, wid, hints);
    XFree(hints);
    return 0;
}

int xutil_set_state(Display *dpy, Window wid, const char *atom_name, int enable) {
    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom target   = XInternAtom(dpy, atom_name, False);

    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type         = ClientMessage;
    ev.xclient.window       = wid;
    ev.xclient.message_type = wm_state;
    ev.xclient.format       = 32;
    ev.xclient.data.l[0]    = enable ? 1 : 0;  /* _NET_WM_STATE_ADD / REMOVE */
    ev.xclient.data.l[1]    = (long)target;
    ev.xclient.data.l[2]    = 0;
    ev.xclient.data.l[3]    = 1;  /* source: application */

    Window root = DefaultRootWindow(dpy);
    if (!XSendEvent(dpy, root, False,
                    SubstructureRedirectMask | SubstructureNotifyMask, &ev)) {
        fprintf(stderr, "winlock: failed to send _NET_WM_STATE for %s\n", atom_name);
        return -1;
    }
    return 0;
}

int xutil_set_decorations(Display *dpy, Window wid, int enable) {
    Atom motif = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

    MotifWmHints hints;
    memset(&hints, 0, sizeof(hints));
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = enable ? 1 : 0;

    XChangeProperty(dpy, wid, motif, motif, 32, PropModeReplace,
                    (unsigned char *)&hints, sizeof(hints) / sizeof(long));
    return 0;
}

int xutil_grab_button(Display *dpy, Window wid, unsigned int button) {
    return XGrabButton(dpy, button, AnyModifier, wid, False,
                       ButtonPressMask, GrabModeAsync, GrabModeAsync,
                       None, None) ? 0 : -1;
}

int xutil_set_opacity(Display *dpy, Window wid, double opacity) {
    Atom atom = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
    unsigned long val = (unsigned long)(opacity * 0xFFFFFFFF);

    XChangeProperty(dpy, wid, atom, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&val, 1);
    return 0;
}

Window xutil_get_frame(Display *dpy, Window wid) {
    Window root = DefaultRootWindow(dpy);
    Window current = wid;
    Window parent;
    Window *children;
    unsigned int nchildren;

    while (1) {
        if (!XQueryTree(dpy, current, &root, &parent, &children, &nchildren))
            return wid;
        if (children) XFree(children);
        if (parent == root || parent == 0)
            return current;
        current = parent;
    }
}

int xutil_get_position(Display *dpy, Window wid, int *x, int *y) {
    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, wid, &attr)) {
        fprintf(stderr, "winlock: cannot get window position\n");
        return -1;
    }

    /* Translate to root coordinates */
    Window child;
    XTranslateCoordinates(dpy, wid, DefaultRootWindow(dpy),
                          0, 0, x, y, &child);
    return 0;
}
