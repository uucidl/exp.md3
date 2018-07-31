compiler_assert(sizeof(unsigned) >= 4);

static struct scissor_rect
xr_xywh(int x, int y, int w, int h)
{
    struct scissor_rect res;
    res.x = x, res.y = y;
    res.w = w, res.h = h;
    return res;
}
static int
xc_parse_hex(const char *p, int n)
{
    int i = 0, len = 0;
    while (len < n) {
        i <<= 4;
        if (p[len] >= 'a' && p[len] <= 'f')
            i += ((p[len] - 'a') + 10);
        else if (p[len] >= 'A' && p[len] <= 'F')
            i += ((p[len] - 'A') + 10);
        else i += (p[len] - '0');
        len++;
    } return i;
}
static unsigned
xc_rgb(int r, int g, int b)
{
    unsigned res = 0;
    res |= (unsigned)((r & 0xFF) << 16);
    res |= (unsigned)((g & 0xFF) << 8);
    res |= (unsigned)((b & 0xFF) << 0);
    return res;
}
static unsigned
xc_rgb_hex(const char *rgb)
{
    const char *c = (*rgb == '#') ? rgb+1: rgb;
    int r = xc_parse_hex(c+0, 2);
    int g = xc_parse_hex(c+2, 2);
    int b = xc_parse_hex(c+4, 2);
    return xc_rgb(r, g, b);
}
static void
xs_setup(struct xsurface *s, Display *dpy, Window root, int screen, int w, int h)
{
    s->w = w, s->h = h;
    s->dpy = dpy;
    s->screen = screen;
    s->root = root;
    s->gc = XCreateGC(dpy, root, 0, NULL);
    s->clip = (struct scissor_rect){0,0,w,h};

    s->state.fnt = 0;
    s->state.offx = 0;
    s->state.offy = 0;
    s->state.line_thickness = 1;
    s->state.line_style = XLINE_SOLID;
    s->state.background = xc_rgb(255,255,255);
    s->state.foreground = xc_rgb(0,0,0);
    XSetLineAttributes(dpy, s->gc, 1, LineSolid, CapButt, JoinMiter);
}
static struct xsurface*
xs_create(Display *dpy, Window root, int screen, int w, int h)
{
    struct xsurface *s = xcalloc(1, szof(struct xsurface));
    xs_setup(s, dpy, root, screen, w, h);
    s->drawable = XCreatePixmap(dpy, root, (unsigned)s->w, (unsigned)s->h,
        (unsigned)DefaultDepth(dpy, screen));
    return s;
}
static struct xsurface*
xs_load(Display *dpy, Window root, int screen, const struct ximage_def *def)
{
    struct xsurface *s = xcalloc(1, szof(struct xsurface));
    char *data = xcalloc(def->w, def->h*4);
    for (int y = 0; y < def->h; ++y) {
        for (int x = 0; x < def->w; ++x) {
            unsigned char sym = (unsigned char)def->data[y][x];
            unsigned long *dst = (unsigned long*)(void*)(data+(x+y*def->w)*4);
            unsigned long col = 0;
            if (sym == ' ')
                col = def->none;
            else col = def->cmap[sym];
            memcpy(dst, &col, 4);
        }
    }
    xs_setup(s, dpy, root, screen, def->w, def->h);
    XImage *img = XCreateImage(dpy, CopyFromParent, 24, ZPixmap, 0,
        data, (unsigned)def->w, (unsigned)def->h, 32, 0);
    s->drawable = XCreatePixmap(s->dpy, s->root, (unsigned)s->w, (unsigned)s->h,
        (unsigned int)DefaultDepth(s->dpy, screen));
    XPutImage(dpy, s->drawable, s->gc, img, 0,0,0,0, (unsigned)s->w, (unsigned)s->h);
    XDestroyImage(img);
    return s;
}
static void
xs_resize(struct xsurface *s, int w, int h)
{
    if(!s) return;
    if (s->w == w && s->h == h) return;
    s->w = w; s->h = h;
    if(s->drawable) XFreePixmap(s->dpy, s->drawable);
    s->drawable = XCreatePixmap(s->dpy, s->root, (unsigned)w, (unsigned)h,
        (unsigned)DefaultDepth(s->dpy, s->screen));
}
static void
xs_clear(struct xsurface *s, unsigned long color)
{
    XSetForeground(s->dpy, s->gc, color);
    XFillRectangle(s->dpy, s->drawable, s->gc, 0, 0, (unsigned)s->w, (unsigned)s->h);
}
static void
xs_copy(Drawable target, struct xsurface *s, unsigned w, unsigned h)
{
    XCopyArea(s->dpy, s->drawable, target, s->gc, 0, 0, w, h, 0, 0);
}
static void
xs_blit(struct xsurface *d, const struct xsurface *s,
    int dx, int dy, int sx, int sy, int w, int h)
{
    XCopyArea(d->dpy, s->drawable, d->drawable, d->gc, sx, sy, (unsigned)w, (unsigned)h, dx, dy);
}
static void
xs_del(struct xsurface *s)
{
    if (!s) return;
    XFreePixmap(s->dpy, s->drawable);
    XFreeGC(s->dpy, s->gc);
    free(s);
}
static void
xs_scissor(struct xsurface *s, int x, int y, int w, int h)
{
    x += s->state.offx;
    y += s->state.offy;

    XRectangle clip_rect;
    clip_rect.x = (short)(x-1);
    clip_rect.y = (short)(y-1);
    clip_rect.width = (unsigned short)(w+2);
    clip_rect.height = (unsigned short)(h+2);

    s->clip.x = x, s->clip.y = y;
    s->clip.w = w, s->clip.h = h;
    XSetClipRectangles(s->dpy, s->gc, 0, 0, &clip_rect, 1, Unsorted);
}
static void
xs_identity(struct xsurface *s)
{
    s->state.offx = 0;
    s->state.offy = 0;
}
static void
xs_translate(struct xsurface *s, int x, int y)
{
    s->state.offx += x;
    s->state.offy += y;
}
static enum xline_style
xs_line_style(struct xsurface *s, enum xline_style style)
{
    enum xline_style old = s->state.line_style;
    s->state.line_style = style;
    return old;
}
static int
xs_line_thickness(struct xsurface *s, int size)
{
    int old = s->state.line_thickness;
    s->state.line_thickness = size;
    return old;
}
static unsigned
xs_color(struct xsurface *s, unsigned col)
{
    unsigned old = s->state.foreground;
    s->state.foreground = col;
    return old;
}
static unsigned
xs_color_background(struct xsurface *s, unsigned col)
{
    unsigned old = s->state.background;
    s->state.background = col;
    return old;
}
static unsigned
xs_color_foreground(struct xsurface *s, unsigned col)
{
    unsigned old = s->state.foreground;
    s->state.foreground = col;
    return old;
}
static void
xs_swap_colors(struct xsurface *s)
{
    unsigned tmp = s->state.foreground;
    s->state.foreground = s->state.background;
    s->state.background = tmp;
}
static const struct xfont*
xs_font(struct xsurface *s, const struct xfont *fnt)
{
    const struct xfont *old = s->state.fnt;
    s->state.fnt = fnt;
    return old;
}
static void
xs_update_color(const struct xsurface *s)
{
    XSetBackground(s->dpy, s->gc, s->state.background);
    XSetForeground(s->dpy, s->gc, s->state.foreground);
}
static void
xs_update_line_style(const struct xsurface *surf)
{
    short s = LineSolid;
    if (surf->state.line_style == XLINE_DASHED) {
        static const char dash_list[] = {1,1,1,1};
        XSetDashes(surf->dpy, surf->gc, 0, dash_list, 4);
        s = LineOnOffDash;
    } XSetLineAttributes(surf->dpy, surf->gc, (unsigned)surf->state.line_thickness,
        s, CapButt, JoinMiter);
}
static void
xs_stroke_line(const struct xsurface *s, int x0, int y0, int x1, int y1)
{
    x0 += s->state.offx;
    y0 += s->state.offy;
    x1 += s->state.offx;
    y1 += s->state.offy;

    xs_update_color(s);
    xs_update_line_style(s);
    XDrawLine(s->dpy, s->drawable, s->gc, x0, y0, x1, y1);
}
static void
xs_stroke_rect(const struct xsurface* s, int x, int y, int w, int h, int r)
{
    x += s->state.offx;
    y += s->state.offy;

    xs_update_color(s);
    xs_update_line_style(s);
    if (r == 0) {
        XDrawRectangle(s->dpy, s->drawable, s->gc, x, y, (unsigned)w, (unsigned)h);
        return;
    }
    {int xc = x + r;
    int yc = y + r;
    int wc = (int)(w - 2 * r);
    int hc = (int)(h - 2 * r);

    XDrawLine(s->dpy, s->drawable, s->gc, xc, y, xc+wc, y);
    XDrawLine(s->dpy, s->drawable, s->gc, x+w, yc, x+w, yc+hc);
    XDrawLine(s->dpy, s->drawable, s->gc, xc, y+h, xc+wc, y+h);
    XDrawLine(s->dpy, s->drawable, s->gc, x, yc, x, yc+hc);

    XDrawArc(s->dpy, s->drawable, s->gc, xc + wc - r, y,
        (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
    XDrawArc(s->dpy, s->drawable, s->gc, x, y,
        (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
    XDrawArc(s->dpy, s->drawable, s->gc, x, yc + hc - r,
        (unsigned)r*2, (unsigned)(2*r), 180 * 64, 90 * 64);
    XDrawArc(s->dpy, s->drawable, s->gc, xc + wc - r, yc + hc - r,
        (unsigned)r*2, (unsigned)(2*r), -90 * 64, 90 * 64);}
}
static void
xs_stroke_circle(const struct xsurface *s, int x, int y, int w, int h)
{
    x += s->state.offx;
    y += s->state.offy;

    xs_update_color(s);
    xs_update_line_style(s);
    XDrawArc(s->dpy, s->drawable, s->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}
static void
xs_fill_rect(const struct xsurface* s, int x, int y, int w, int h, int r)
{
    x += s->state.offx;
    y += s->state.offy;

    xs_update_color(s);
    if (r == 0) {
        XFillRectangle(s->dpy, s->drawable, s->gc, x, y, (unsigned)w, (unsigned)h);
        return;
    }
    {int xc = x + r;
    int yc = y + r;
    int wc = (int)(w - 2 * r);
    int hc = (int)(h - 2 * r);

    XPoint pnts[12];
    pnts[0].x = (short)x;
    pnts[0].y = (short)yc;
    pnts[1].x = (short)xc;
    pnts[1].y = (short)yc;
    pnts[2].x = (short)xc;
    pnts[2].y = (short)y;

    pnts[3].x = (short)(xc + wc);
    pnts[3].y = (short)(y);
    pnts[4].x = (short)(xc + wc);
    pnts[4].y = (short)(yc);
    pnts[5].x = (short)(x + w);
    pnts[5].y = (short)(yc);

    pnts[6].x = (short)(x + w);
    pnts[6].y = (short)(yc + hc);
    pnts[7].x = (short)(xc + wc);
    pnts[7].y = (short)(yc + hc);
    pnts[8].x = (short)(xc + wc);
    pnts[8].y = (short)(y + h);

    pnts[9].x = (short)(xc);
    pnts[9].y = (short)(y + h);
    pnts[10].x =(short)( xc);
    pnts[10].y =(short)( yc + hc);
    pnts[11].x =(short)( x);
    pnts[11].y =(short)( yc + hc);

    XFillPolygon(s->dpy, s->drawable, s->gc, pnts, 12, Convex, CoordModeOrigin);
    XFillArc(s->dpy, s->drawable, s->gc, xc + wc - r, y,
        (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
    XFillArc(s->dpy, s->drawable, s->gc, x, y,
        (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
    XFillArc(s->dpy, s->drawable, s->gc, x, yc + hc - r,
        (unsigned)r*2, (unsigned)(2*r), 180 * 64, 90 * 64);
    XFillArc(s->dpy, s->drawable, s->gc, xc + wc - r, yc + hc - r,
        (unsigned)r*2, (unsigned)(2*r), -90 * 64, 90 * 64);}
}
static void
xs_fill_circle(const struct xsurface *s, int x, int y, int w, int h)
{
    x += s->state.offx;
    y += s->state.offy;

    xs_update_color(s);
    XFillArc(s->dpy, s->drawable, s->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}
static void
xs_draw_text(const struct xsurface *s, int x, int y, int w, int h,
    const char *text, int len)
{
    x += s->state.offx;
    y += s->state.offy;
    xs_update_color(s);

    const struct xfont *font = s->state.fnt;
    int tx = (int)x, ty = (int)y + font->ascent;
    if(font->set)
        XmbDrawString(s->dpy,s->drawable,font->set,s->gc,tx,ty,(const char*)text,(int)len);
    else XDrawString(s->dpy, s->drawable, s->gc, tx, ty, (const char*)text, (int)len);
}
static struct xfont*
xf_create(Display *dpy, const char *name)
{
    int n;
    char *def, **missing;
    struct xfont *font = xcalloc(1, szof(struct xfont));
    font->set = XCreateFontSet(dpy, name, &missing, &n, &def);
    if(missing) {
        while(n--)
            fprintf(stderr, "missing fontset: %s\n", missing[n]);
        XFreeStringList(missing);
    }
    if(font->set) {
        XFontStruct **xfonts;
        char **font_names = 0;
        XExtentsOfFontSet(font->set);
        n = XFontsOfFontSet(font->set, &xfonts, &font_names);
        while(n--) {
            font->ascent = max(font->ascent, (*xfonts)->ascent);
            font->descent = max(font->descent,(*xfonts)->descent);
            xfonts++;
        }
    } else {
        if(!(font->xfont = XLoadQueryFont(dpy, name)) &&
           !(font->xfont = XLoadQueryFont(dpy, "fixed"))) {
            free(font);
            return 0;
        }
        font->ascent = font->xfont->ascent;
        font->descent = font->xfont->descent;
    }
    font->h = font->ascent + font->descent;
    return font;
}
static int
xf_get_text_width(const struct xfont *f, const char *text, const char *end)
{
    XRectangle r;
    if(!f || !text) return 0;
    int len = (end) ? cast(int, end-text): (int)strlen(text);
    if(f->set) {
        XmbTextExtents(f->set, text, len, NULL, &r);
        return r.width;
    } else {
        int w = XTextWidth(f->xfont, text, len);
        return w;
    }
}
static struct extend
xf_text_measure(const struct xfont *f, const char *txt, const char *end)
{
    struct extend m;
    m.w = xf_get_text_width(f, txt, end);
    m.h = f->h;
    return m;
}
static void
xf_del(Display *dpy, struct xfont *font)
{
    if(!font) return;
    if(font->set) XFreeFontSet(dpy, font->set);
    else XFreeFont(dpy, font->xfont);
    free(font);
}
