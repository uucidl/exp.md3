#define SYS_MAX_INPUT 1024
#define SYS_MAX_KEYS 512

struct sys_int2 {int x,y;};
struct sys_window {
    const char *title;
    int w, h;
};
struct sys_button {
    unsigned down:1;
    unsigned pressed:1;
    unsigned released:1;
    unsigned doubled:1;
};
struct sys_mouse {
    struct sys_int2 pos;
    struct sys_int2 pos_last;
    struct sys_int2 d;
    float scroll_delta;
    struct sys_button left;
    struct sys_button right;
    long timestamp;
};
struct sys_time {
    long min_frame;
    long delta;
    long started;
    long ended;
};
enum sys_key {
    SYS_KEY_BACKSPACE = 0,
    SYS_KEY_ACTIVATE,
    SYS_KEY_NEXT_WIDGET,
    SYS_KEY_LEFT,
    SYS_KEY_RIGHT,
    SYS_KEY_UP,
    SYS_KEY_DOWN,
    SYS_KEY_ESC,
    SYS_KEY_MAX,
};
struct sys {
    /* systems */
    int quit;
    struct sys_window win;
    struct sys_time time;
    struct sys_mouse mouse;
    struct xsurface *surf;
    struct xsurface *popup;
    struct sys_button keys[SYS_KEY_MAX];
    char text[SYS_MAX_INPUT];
    int text_len;

    /* platform */
    Display *dpy;
    Window root;
    Visual *vis;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window window;
    int screen;
    Atom wm_delete_window;
};
static void sys_init(struct sys *s);
static void sys_poll(struct sys *s);
static void sys_push(struct sys *s);
static void sys_shutdown(struct sys *s);
