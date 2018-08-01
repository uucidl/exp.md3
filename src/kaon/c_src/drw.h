struct xfont {
    int ascent;
    int descent;
    int h;
    XFontSet set;
    XFontStruct *xfont;
};
struct scissor_rect {
    int x,y,w,h;
};
enum xline_style {
    XLINE_SOLID,
    XLINE_DASHED
};
struct xstate {
    int offx, offy;
    int line_thickness;
    enum xline_style line_style;
    unsigned background;
    unsigned foreground;
    const struct xfont *fnt;
};
struct xsurface {
    GC gc;
    Display *dpy;
    int screen;
    Window root;
    Drawable drawable;
    int w, h;
    struct scissor_rect clip;
    struct xstate state;
};
struct ximage_def {
    int w, h;
    const unsigned *cmap;
    const char **data;
    unsigned none;
};
/* misc */
static unsigned xc_rgb(int r, int g, int b);
static unsigned xc_rgb_hex(const char *rgb);
static struct scissor_rect xr_xywh(int x, int y, int w, int h);

/* font */
struct extend {int w,h;};
static struct xfont* xf_create(Display *dpy, const char *name);
static struct extend xf_text_measure(const struct xfont *f, const char *txt, const char *end);
static void xf_del(Display *dpy, struct xfont *font);

/* surface */
static struct xsurface* xs_create(Display *dpy, Window root, int screen, int w, int h);
static struct xsurface* xs_load(Display *dpy, Window root, int screen, const struct ximage_def *def);
static void xs_resize(struct xsurface *s, int w, int h);
static void xs_clear(struct xsurface *s, unsigned long color);
static void xs_copy(Drawable target, struct xsurface *s, unsigned w, unsigned h);
static void xs_blit(struct xsurface *d, const struct xsurface *s, int dx, int dy, int sx, int sy, int w, int h);
static void xs_del(struct xsurface *s);
static void xs_scissor(struct xsurface *s, int x, int y, int w, int h);

/* state */
static void xs_identity(struct xsurface *s);
static void xs_translate(struct xsurface *s, int x, int y);
static enum xline_style xs_line_style(struct xsurface *s, enum xline_style style);
static int xs_line_thickness(struct xsurface *s, int size);
static unsigned xs_color(struct xsurface *s, unsigned col);
static unsigned xs_color_background(struct xsurface *s, unsigned col);
static unsigned xs_color_foreground(struct xsurface *s, unsigned col);
static void xs_swap_colors(struct xsurface *s);
static const struct xfont *xs_font(struct xsurface *s, const struct xfont *fnt);

/* draw */
static void xs_stroke_line(const struct xsurface *s, int x0, int y0, int x1, int y1);
static void xs_stroke_rect(const struct xsurface* s, int x, int y, int w, int h, int r);
static void xs_stroke_circle(const struct xsurface *s, int x, int y, int w, int h);
static void xs_fill_rect(const struct xsurface* s, int x, int y, int w, int h, int r);
static void xs_fill_circle(const struct xsurface *s, int x, int y, int w, int h);
static void xs_draw_text(const struct xsurface *s, int x, int y, int w, int h, const char *text, int len);
