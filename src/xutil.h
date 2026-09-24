/*
 * xutil.h - X11 utility functions for winlock
 */

#ifndef WINLOCK_XUTIL_H
#define WINLOCK_XUTIL_H

#include <X11/Xlib.h>

/* Lock window size by setting WM_NORMAL_HINTS min==max.
 * If w/h are <= 0, uses the window's current dimensions. */
int xutil_lock_size(Display *dpy, Window wid, int w, int h);

/* Send _NET_WM_STATE ClientMessage to add/remove a state atom.
 * atom_name: e.g. "_NET_WM_STATE_ABOVE", "_NET_WM_STATE_STICKY" */
int xutil_set_state(Display *dpy, Window wid, const char *atom_name, int enable);

/* Set or remove window decorations via _MOTIF_WM_HINTS. */
int xutil_set_decorations(Display *dpy, Window wid, int enable);

/* Passive grab on a mouse button (blocks the click from reaching the window). */
int xutil_grab_button(Display *dpy, Window wid, unsigned int button);

/* Set window opacity (0.0 = transparent, 1.0 = opaque). */
int xutil_set_opacity(Display *dpy, Window wid, double opacity);

/* Get window position in root coordinates. */
int xutil_get_position(Display *dpy, Window wid, int *x, int *y);

/* Find the WM frame window (direct child of root) for a client window.
 * Returns the client window itself if no frame is found. */
Window xutil_get_frame(Display *dpy, Window wid);

#endif /* WINLOCK_XUTIL_H */
