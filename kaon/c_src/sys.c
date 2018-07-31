#ifndef SYS_DOUBLE_CLICK_LO
  #define SYS_DOUBLE_CLICK_LO 20
#endif
#ifndef SYS_DOUBLE_CLICK_HI
  #define SYS_DOUBLE_CLICK_HI 300
#endif

static long
sys_timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}
static void
sys_sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}
static void
sys_update_button(struct sys_button *b, int down)
{
    int was_down = b->down;
    b->down = down ? 1u: 0u;
    b->pressed = !was_down && down;
    b->released = was_down && !down;
    b->doubled = 0;
}
static void
sys_init(struct sys *s)
{
    s->win.w = !s->win.w ? 800: s->win.w;
    s->win.h = !s->win.h ? 600: s->win.h;
    s->win.title = !s->win.title ? "X11": s->win.title;

    s->dpy = XOpenDisplay(NULL);
    if (!s->dpy) panic("Could not open a display; perhaps $DISPLAY is not set?");
    s->root = DefaultRootWindow(s->dpy);
    s->screen = XDefaultScreen(s->dpy);
    s->vis = XDefaultVisual(s->dpy, s->screen);
    s->cmap = XCreateColormap(s->dpy,s->root,s->vis,AllocNone);

    s->swa.colormap = s->cmap;
    s->swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
        PointerMotionMask | KeymapStateMask;
    s->window = XCreateWindow(s->dpy, s->root, 0, 0, (unsigned)s->win.w,
        (unsigned)s->win.h, 0, XDefaultDepth(s->dpy, s->screen), InputOutput,
        s->vis, CWEventMask | CWColormap, &s->swa);

    XStoreName(s->dpy, s->window, s->win.title);
    XMapWindow(s->dpy, s->window);
    s->wm_delete_window = XInternAtom(s->dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(s->dpy, s->window, &s->wm_delete_window, 1);
    XGetWindowAttributes(s->dpy, s->window, &s->attr);
    s->surf = xs_create(s->dpy, s->root, s->screen, s->win.w, s->win.h);
    s->popup = xs_create(s->dpy, s->root, s->screen, 1, 1);
    XSelectInput(s->dpy, s->window, s->swa.event_mask);
}
static void
sys_clear(struct sys *s)
{
    s->mouse.left.pressed = 0;
    s->mouse.left.released = 0;
    s->mouse.left.doubled = 0;
    s->mouse.right.pressed = 0;
    s->mouse.right.released = 0;
    s->mouse.scroll_delta = 0;
    s->mouse.pos_last = s->mouse.pos;
    s->mouse.d.x = 0;
    s->mouse.d.y = 0;

    for (int i = 0; i < SYS_KEY_MAX; ++i) {
        s->keys[i].released = 0;
        s->keys[i].pressed = 0;
    }
}
static void
sys_poll(struct sys *s)
{
    XClearWindow(s->dpy, s->window);
    sys_clear(s);
    s->text_len = 0;

    pump:; XEvent e;
    XNextEvent(s->dpy, &e);
    do {if (XFilterEvent(&e, s->window)) goto pump;
        switch (e.type) {
        default: break;
        case NoExpose: goto pump;
        case ClientMessage: s->quit = 1; break;
        case Expose: case ConfigureNotify:  {
            /* Window resize handler */
            XWindowAttributes attr;
            XGetWindowAttributes(s->dpy, s->window, &attr);
            xs_resize(s->surf, attr.width, attr.height);
            s->win.w = attr.width, s->win.h = attr.height;
        } break;
        case ButtonPress:
        case ButtonRelease: {
            /* Button handler */
            int down = (e.type == ButtonPress);
            if (e.xbutton.button == Button1) {
                if (down) { /* Double-Click Button handler */
                    long dt = sys_timestamp() - s->mouse.timestamp;
                    if (dt > SYS_DOUBLE_CLICK_LO && dt < SYS_DOUBLE_CLICK_HI)
                        s->mouse.left.doubled = 1;
                    else {
                        sys_update_button(&s->mouse.left, down);
                        s->mouse.timestamp = sys_timestamp();
                    }
                } else sys_update_button(&s->mouse.left, down);
            }
            else if (e.xbutton.button == Button3)
                sys_update_button(&s->mouse.right, down);
            else if (e.xbutton.button == Button4)
                s->mouse.scroll_delta += 1.0f;
            else if (e.xbutton.button == Button5)
                s->mouse.scroll_delta -= 1.0f;
        } break;
        case MotionNotify: {
            /* Mouse motion handler */
            s->mouse.pos.x = e.xmotion.x;
            s->mouse.pos.y = e.xmotion.y;
            s->mouse.d.x = s->mouse.pos.x - s->mouse.pos_last.x;
            s->mouse.d.y = s->mouse.pos.y - s->mouse.pos_last.y;
        } break;
        case KeyPress:
        case KeyRelease: {
            /* Key handler */
            char buf[32] = {0};
            KeySym keysym = 0;
            int ret, down = (e.type == KeyPress);
            KeySym *code = XGetKeyboardMapping(s->dpy, (KeyCode)e.xkey.keycode, 1, &ret);
            if (*code == XK_BackSpace)
                sys_update_button(&s->keys[SYS_KEY_BACKSPACE], down);
            else if (*code == XK_Return)
                sys_update_button(&s->keys[SYS_KEY_ACTIVATE], down);
            else if (*code == XK_Tab)
                sys_update_button(&s->keys[SYS_KEY_NEXT_WIDGET], down);
            else if (*code == XK_Left)
                sys_update_button(&s->keys[SYS_KEY_LEFT], down);
            else if (*code == XK_Right)
                sys_update_button(&s->keys[SYS_KEY_RIGHT], down);
            else if (*code == XK_Up)
                sys_update_button(&s->keys[SYS_KEY_UP], down);
            else if (*code == XK_Down)
                sys_update_button(&s->keys[SYS_KEY_DOWN], down);
            else if (*code == XK_Escape)
                sys_update_button(&s->keys[SYS_KEY_ESC], down);
            else if (down) {
                if (XLookupString((XKeyEvent*)&e, buf, 32, &keysym, NULL) != NoSymbol) {
                    int len = utf8_len(buf, 0);
                    if (s->text_len + len < szof(s->text)) {
                        memcpy(s->text + s->text_len, buf, cast(size_t,len));
                        s->text_len += len;
                    }
                }
            }
            XFree(code);
        } break;}
    } while (XCheckWindowEvent(s->dpy, s->window, s->swa.event_mask, &e));
    s->time.started = sys_timestamp();
}
static void
sys_push(struct sys *s)
{
    xs_copy(s->window, s->surf, (unsigned)s->surf->w, (unsigned)s->surf->h);
    XFlush(s->dpy);

    s->time.ended = sys_timestamp();
    s->time.delta = s->time.ended - s->time.started;
    if (s->time.delta < s->time.min_frame)
        sys_sleep_for(s->time.min_frame - s->time.delta);
}
static void
sys_shutdown(struct sys *s)
{
    xs_del(s->surf);
    XUnmapWindow(s->dpy, s->window);
    XFreeColormap(s->dpy, s->cmap);
    XDestroyWindow(s->dpy, s->window);
    XCloseDisplay(s->dpy);
}
