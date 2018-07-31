/* ---------------------------------------------------------------------------
 *                                  IDs
 * --------------------------------------------------------------------------- */
#define ui_push_unique_index_id(s,idx) ui_push_id(s, murmur_hash(FILE_LINE, strlen(FILE_LINE),(uintptr_t)idx))
#define ui_push_unique_id(s) ui_push_unique_index_id(s,0)
#define ui_push_ptr_id(s, p) ui_push_id(s, (uintptr_t)p)

static ui_bool
ui_id_eq(const struct ui_id *a, const struct ui_id *b)
{
    return a->lo == b->lo && a->hi == b->hi;
}
static struct ui_id
ui_gen_id(struct ui_id_stk *s)
{
    struct ui_id id;
    struct ui_id *entry;
    assert(s && s->top);

    entry = &s->stk[s->top-1];
    id.lo = ++entry->lo;
    id.hi = entry->hi;
    return id;
}
static void
ui_set_id(struct ui_id_stk *s, const struct ui_id *id)
{
    assert(s->top > 0);
    s->stk[s->top-1] = *id;
}
static void
ui_push_id(struct ui_id_stk *s, uintptr_t id)
{
    struct ui_id *entry = 0;
    assert(s->top < UI_ID_STACK_DEPTH);
    entry = s->stk + s->top++;
    entry->hi = id;
    entry->lo = 0;
}
static void
ui_pop_id(struct ui_id_stk *s)
{
    assert(s->top > 1);
    s->top--;
}
static struct ui_id
ui_peek_id(struct ui_id_stk *s)
{
    assert(s);
    assert(s->top);
    return s->stk[s->top-1];
}

/* ---------------------------------------------------------------------------
 *                                  Box
 * --------------------------------------------------------------------------- */
static void
ui_anchor_left_right(struct ui_box *b, int left, int right)
{
    assert(b);
    if (!b) return;
    b->left = left;
    b->right = right;
    b->width = b->right - b->left;
    b->center_x = b->left + (b->width >> 1);
}
static void
ui_anchor_left_width(struct ui_box *b, int left, int width)
{
    assert(b);
    if (!b) return;
    b->left = left;
    b->width = width;
    b->right = b->left + width;
    b->center_x = b->left + (b->width >> 1);
}
static void
ui_anchor_right_width(struct ui_box *b, int right, int width)
{
    assert(b);
    if (!b) return;
    b->right = right;
    b->width = width;
    b->left = b->right - width;
    b->center_x = b->left + (b->width >> 1);
}
static void
ui_anchor_center_width(struct ui_box *b, int center, int width)
{
    assert(b);
    if (!b) return;
    b->width = width;
    b->center_x = center;
    b->right = b->center_x + (width >> 1);
    b->left = b->right - width;
}
static void
ui_anchor_left_center(struct ui_box *b, int left, int center)
{
    assert(b);
    if (!b) return;
    b->left = left;
    b->center_x = center;
    b->width = (center - left) << 1;
    b->right = b->left + b->width;
}
static void
ui_anchor_right_center(struct ui_box *b, int right, int center)
{
    assert(b);
    if (!b) return;
    b->right = right;
    b->center_x = center;
    b->width = (right - center) << 1;
    b->left  = b->right - b->width;
}
static void
ui_anchor_top_bottom(struct ui_box *b, int top, int bottom)
{
    assert(b);
    if (!b) return;
    b->top = top;
    b->bottom = bottom;
    b->height = bottom - top;
    b->center_y = top + (b->height >> 1);
}
static void
ui_anchor_top_height(struct ui_box *b, int top, int height)
{
    assert(b);
    if (!b) return;
    b->top = top;
    b->height = height;
    b->bottom = top + height;
    b->center_y = b->top + (b->height >> 1);
}
static void
ui_anchor_bottom_height(struct ui_box *b, int bottom, int height)
{
    assert(b);
    if (!b) return;
    b->height = height;
    b->bottom = bottom;
    b->top = bottom - height;
    b->center_y = b->top + (b->height >> 1);
}
static void
ui_anchor_center_height(struct ui_box *b, int center, int height)
{
    assert(b);
    if (!b) return;
    b->height = height;
    b->center_y = center;
    b->top = center - (b->height >> 1);
    b->bottom = b->top + b->height;
}
static void
ui_anchor_top_center(struct ui_box *b, int top, int center)
{
    assert(b);
    if (!b) return;
    b->top = top;
    b->center_y = center;
    b->height = (center - top) << 1;
    b->bottom = b->top + b->height;
}
static void
ui_anchor_bottom_center(struct ui_box *b, int bottom, int center)
{
    assert(b);
    if (!b) return;
    b->center_y = center;
    b->bottom = bottom;
    b->height = (bottom - center) << 1;
    b->top = b->bottom - b->height;
}
static void
ui_anchor_xywh(struct ui_box *b, int left, int top, int width, int height)
{
    ui_anchor_left_width(b, left, width);
    ui_anchor_top_height(b, top, height);
}

/* ---------------------------------------------------------------------------
 *                                  Panel
 * --------------------------------------------------------------------------- */
static void
ui_panel_hot(struct ui_ctx *ctx, struct ui_panel *p)
{
    assert(ctx && p);
    const struct sys *s = ctx->sys;
    const int mx = s->mouse.pos.x;
    const int my = s->mouse.pos.y;
    const struct ui_box *b = &p->box;

    if (p->unselectable) return;
    if (p->parent && !p->parent->hovered) return;
    if (!inbox(mx,my, b->left,b->top,b->width,b->height))
        return;

    p->hovered = ui_true;
    ctx->hot = p->id;
}
static void
ui_panel_input(struct ui_ctx *ctx, struct ui_panel *p)
{
    assert(ctx && p);
    assert(ctx->sys);
    struct sys *sys = ctx->sys;

    if (ctx->disabled) return;
    if (ui_id_eq(&p->id, &ctx->active) &&
        ui_id_eq(&ctx->active, &ctx->origin))
        p->down = ui_true;
    if (ui_id_eq(&p->id, &ctx->active)) {
        if (sys->keys[SYS_KEY_NEXT_WIDGET].pressed) {
            sys->keys[SYS_KEY_NEXT_WIDGET].pressed = ui_false;
            ctx->activate_next = ui_true;
        }
        if (sys->keys[SYS_KEY_ACTIVATE].pressed) {
            p->clicked = ui_true;
            p->down = ui_true;
        }
    }
    if (sys->mouse.d.x || sys->mouse.d.y) {
        if (ui_id_eq(&p->id, &ctx->active) &&
            ui_id_eq(&ctx->active, &ctx->origin))
            p->dragged = ui_true;
    }
    if (p->hovered) {
        p->doubled = sys->mouse.left.doubled;
        p->down = sys->mouse.left.down;
        p->pressed = sys->mouse.left.pressed;
        p->released = sys->mouse.left.released;

        if (p->pressed)
            p->drag_begin = ui_true;
        if (p->released) {
            if (ui_id_eq(&ctx->hot, &ctx->active))
                p->clicked = ui_true;
            if (ui_id_eq(&p->id, &ctx->origin))
                p->drag_end = ui_true;
        }
    }
}
static enum ui_state
ui_panel_state(const struct ui_ctx *ctx, const struct ui_panel *p)
{
    enum ui_state state;
    if (ctx->disabled)
        state = UI_DISABLED;
    else if (ui_id_eq(&p->id, &ctx->active) && !ctx->activate_next)
        state = UI_FOCUSED;
    else state = UI_NORMAL;
    return state;
}
static void
ui_panel_begin(struct ui_ctx *ctx, struct ui_panel *p, struct ui_panel *parent)
{
    assert(ctx && p);
    p->parent = parent;
    p->id = ui_gen_id(&ctx->ids);
    if (ctx->activate_next && p->focusable && !ctx->disabled) {
        ctx->activate_next = ui_false;
        ctx->active = p->id;
    }
    ui_panel_hot(ctx, p);
    ui_panel_input(ctx, p);
    p->state = ui_panel_state(ctx, p);
}
static void
ui_panel_end(struct ui_panel *p)
{
    struct ui_panel *root = p->parent;
    if (!root) return;
    root->max_x = max(root->max_x, p->box.right);
    root->max_y = max(root->max_y, p->box.bottom);
}
static void
ui_panel(struct ui_ctx *ctx, struct ui_panel *p, struct ui_panel *parent)
{
    ui_panel_begin(ctx, p, parent);
    ui_panel_end(p);
}

/* ---------------------------------------------------------------------------
 *                                  Popup
 * --------------------------------------------------------------------------- */
static ui_bool
ui_popup_is_active(struct ui_ctx *ctx, struct ui_id id)
{
    const struct ui_popup *p = &ctx->popup;
    if (!p->active || !ui_id_eq(&ctx->popup.id, &id))
        return ui_false;
    return ui_true;
}
static void
ui_popup_activate(struct ui_ctx *ctx, struct ui_id id,
    enum ui_popup_type type)
{
    struct sys *sys = ctx->sys;
    struct ui_popup *p = &ctx->popup;
    memset(p, 0, sizeof(*p));

    ctx->active = (struct ui_id){.hi = id.lo};
    ctx->origin = ctx->active;
    ctx->hot = ctx->active;

    p->surf = sys->popup;
    p->active = ui_true;
    p->seq = ctx->seq;
    p->type = type;
    p->id = id;
    sys_clear(sys);
}
static void
ui_popup_deactivate(struct ui_ctx *ctx, struct ui_id id)
{
    struct sys *sys = ctx->sys;
    struct ui_popup *p = &ctx->popup;
    if (!p->active || !ui_id_eq(&p->id, &id))
        return;

    ctx->active = ctx->root.id;
    ctx->origin = ctx->root.id;
    ctx->hot = ctx->root.id;

    p->seq = ctx->seq - 1;
    p->active = ui_false;
    sys_clear(sys);
}
static ui_bool
ui_popup_resize(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_id id, int w, int h)
{
    struct sys *sys = ctx->sys;
    struct ui_popup *p = &ctx->popup;
    if (!ui_popup_is_active(ctx, id))
        return ui_false;

    p->w = w, p->h = h;
    memset(pan, 0, sizeof(*pan));
    ui_anchor_left_width(&pan->box, p->x, w);
    ui_anchor_top_height(&pan->box, p->y, h);
    ui_panel_begin(ctx, pan, 0);

    switch (p->type) {
    default: assert(0); break;
    case UI_POPUP_BLOCKING: break;
    case UI_POPUP_NON_BLOCKING: {
        if (sys->mouse.left.pressed && !pan->pressed) {
            ui_popup_deactivate(ctx, id);
            return ui_false;
        }
    } break;}

    /* setup drawing */
    xs_identity(p->surf);
    xs_resize(p->surf, w, h);
    xs_scissor(p->surf, 0, 0, w, h);
    xs_clear(p->surf, xc_rgb(255,255,255));
    xs_line_style(p->surf, XLINE_SOLID);
    xs_line_thickness(p->surf, 1);
    xs_color_background(p->surf, xc_rgb(212,208,200));
    xs_color(p->surf, xc_rgb(0,0,0));
    xs_font(p->surf, res_font(ctx->res));
    xs_stroke_rect(p->surf, 0, 0, w, h, 0);
    xs_translate(p->surf, -p->x, -p->y);
    return ui_true;
}
static void
ui_popup_end(struct ui_ctx *ctx, struct ui_panel *pan)
{
    struct sys *sys = ctx->sys;
    struct ui_popup *p = &ctx->popup;
    ui_pop_id(&ctx->ids);
    ctx->clip = p->clip;
    sys->surf = p->old;
}
static ui_bool
ui_popup_begin(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_id id, int x, int y, int w, int h)
{
    struct sys *sys = ctx->sys;
    struct ui_popup *p = &ctx->popup;
    if (!ui_popup_is_active(ctx, id))
        return ui_false;

    /* close non-blocking popups by escape key */
    if (sys->keys[SYS_KEY_ESC].pressed) {
        switch (p->type) {
        default: assert(0); break;
        case UI_POPUP_BLOCKING: break;
        case UI_POPUP_NON_BLOCKING: {
            ui_popup_deactivate(ctx, id);
            return ui_false;
        } break;}
    }
    p->id = id;
    p->seq = ctx->seq;
    p->x = x, p->y = y;
    p->clip = ctx->clip;
    p->old = sys->surf;

    sys->surf = p->surf;
    ui_push_id(&ctx->ids, id.lo);
    xs_scissor(p->surf, 0, 0, w, h);
    ctx->clip = xr_xywh(p->x, p->y, w, h);
    if (!ui_popup_resize(ctx, pan, id, w, h)) {
        ui_popup_end(ctx, pan);
        return ui_false;
    } return ui_true;
}

/* ---------------------------------------------------------------------------
 *                                  Context
 * --------------------------------------------------------------------------- */
static struct ui_panel*
ui_begin(struct ui_ctx *ctx)
{
    struct sys *sys = ctx->sys;
    res_activate_font(ctx->res, FONT_DEFAULT);
    ctx->layout = UI_LAYOUT_STRETCH;
    ctx->disabled = ui_false;
    ctx->seq += 1;

    /* reset IDs */
    ctx->ids.top = 1;
    ctx->ids.stk[0].lo = 0;
    ctx->ids.stk[0].hi = 0;

    /* reset surface */
    xs_identity(sys->surf);
    xs_line_style(sys->surf, XLINE_SOLID);
    xs_line_thickness(sys->surf, 1);
    xs_color_background(sys->surf, xc_rgb(212,208,200));
    xs_color(sys->surf, xc_rgb(0,0,0));
    xs_font(sys->surf, res_font(ctx->res));

    /* tree */
    struct ui_panel *pan = &ctx->root;
    pan->id = ui_gen_id(&ctx->ids);
    ctx->root.parent = 0;

    /* input */
    if (sys->mouse.left.pressed) {
        ctx->origin = ctx->hot;
        ctx->active = ctx->origin;
    }
    if (sys->mouse.left.released)
        ctx->origin = ctx->root.id;

    /* root */
    ui_anchor_left_width(&pan->box, 0, sys->win.w);
    ui_anchor_top_height(&pan->box, 0, sys->win.h);
    if (ctx->popup.active == ui_false)
        ui_panel_hot(ctx, pan);
    else pan->hovered = ui_false;
    xs_scissor(sys->surf, 0, 0, sys->win.w, sys->win.h);
    ctx->clip = xr_xywh(0, 0, sys->win.w, sys->win.h);
    return &ctx->root;
}
static void
ui_end(struct ui_ctx *ctx)
{
    struct ui_popup *p = &ctx->popup;
    if (p->active == 0 || p->seq != ctx->seq) {
        p->active = ui_false;
        return;
    }
    const struct sys *sys = ctx->sys;
    xs_blit(sys->surf, p->surf, p->x, p->y, 0,0, p->w, p->h);
}
static void
ui_scissor(struct ui_ctx *ctx, int x, int y, int w, int h)
{
    const struct sys *sys = ctx->sys;
    struct xsurface *surf = sys->surf;
    xs_scissor(surf, x, y, w, h);
    ctx->clip = surf->clip;
}
static struct scissor_rect
ui_clip_begin(struct ui_ctx *ctx, int x, int y, int w, int h)
{
    struct scissor_rect p = ctx->clip;
    ctx->clip.x = max(p.x, x);
    ctx->clip.y = max(p.y, y);
    ctx->clip.w = min(p.x + p.w, x + w) - ctx->clip.x;
    ctx->clip.h = min(p.y + p.h, y + h) - ctx->clip.y;
    ctx->clip.w = max(0, ctx->clip.w);
    ctx->clip.h = max(0, ctx->clip.h);
    ui_scissor(ctx, ctx->clip.x, ctx->clip.y, ctx->clip.w, ctx->clip.h);
    return p;
}
static void
ui_clip_end(struct ui_ctx *ctx, struct scissor_rect clip)
{
    ui_scissor(ctx, clip.x, clip.y, clip.w, clip.h);
}
static enum ui_layout
ui_layout(struct ui_ctx *ctx, enum ui_layout layout)
{
    enum ui_layout old = ctx->layout;
    ctx->layout = layout;
    return old;
}
static void
ui_enable(struct ui_ctx *ctx)
{
    ctx->disabled = ui_false;
}
static void
ui_disable(struct ui_ctx *ctx)
{
    ctx->disabled = ui_true;
}
/* ---------------------------------------------------------------------------
 *                              Util:Text
 * --------------------------------------------------------------------------- */
struct ui_text_bounds {
    int len, width;
    const char *end;
};
static struct ui_text_bounds
ui_text_fit(int space, const struct xfont *xf, const char *txt, const char *end)
{
    struct ui_text_bounds res = {0};
    if (!space) return res;

    struct utf8_iter it;
    for (utf8_begin(&it, txt, end); utf8_next(&it);) {
        struct extend ext = xf_text_measure(xf, txt, it.rune_end);
        if (ext.w > space)
            return res;

        res.end = it.rune_end;
        res.len += it.rune_len;
        res.width = ext.w;
    } return res;
}

struct ui_text_window_bounds {int off, len, width;};
static struct ui_text_window_bounds
ui_text_fit_window(int space, const struct xfont *xf, const char *buf, int len)
{
    struct ui_text_window_bounds res = {0};
    res.len = len, res.off = 0;
    if (!space) return res;

    restart:; struct utf8_iter it;
    for (utf8_begin(&it, buf + res.off, buf + len); utf8_next(&it);) {
        struct extend ext = xf_text_measure(xf, buf + res.off, it.rune_end);
        if (ext.w > space) {
            res.off = cast(int, it.rune_begin - (buf + res.off));
            res.len = len - res.off;
            goto restart;
        } res.width = ext.w;
    } return res;
}

/* ---------------------------------------------------------------------------
 *                              Util:Record
 * --------------------------------------------------------------------------- */
static void
ui_record_store(struct ui_record *rec, struct ui_ctx *ctx, struct ui_panel *pan)
{
    rec->ctx = ctx;
    rec->pan = pan;
    rec->max_x = pan->max_x;
    rec->max_y = pan->max_y;
    rec->id_begin = ui_peek_id(&ctx->ids);
}
static void
ui_record_restore(struct ui_record *rec)
{
    struct ui_ctx *ctx = rec->ctx;
    struct ui_panel *pan = rec->pan;
    ui_set_id(&ctx->ids, &rec->id_begin);
    pan->max_x = rec->max_x;
    pan->max_y = rec->max_y;
}

/* ---------------------------------------------------------------------------
 *                              Util: Drag
 * --------------------------------------------------------------------------- */
static float
ui_drag_ratio(struct ui_ctx *ctx, enum ui_orientation orient, int cur, int min, float total)
{
    int off = 0;
    const struct sys *sys = ctx->sys;
    switch (orient) {
    default: assert(0); break;
    case UI_VERTICAL: off = cur + sys->mouse.d.y; break;
    case UI_HORIZONTAL: off = cur + sys->mouse.d.x; break;}

    float ratio = (float)(off - min)/total;
    return clamp(0, ratio, 1.0f);
    return 0;
}

/* ---------------------------------------------------------------------------
 *                                  Button
 * --------------------------------------------------------------------------- */
static void
ui_button_begin(struct ui_ctx *ctx, struct ui_panel *btn, struct ui_panel *parent)
{
    ui_panel_begin(ctx, btn, parent); {
        const struct sys *sys = ctx->sys;
        const struct ui_box *b = &btn->box;

        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_color(sys->surf, xc_rgb(212,208,200));
        xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);

        switch (btn->state) {
        default: assert(0); break;
        case UI_DISABLED:
        case UI_FOCUSED:
        case UI_NORMAL: {
            if (!btn->down) {
                xs_color(sys->surf, xc_rgb(255,255,255));
                xs_stroke_line(sys->surf, b->left, b->top, b->right, b->top);
                xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom);

                xs_color(sys->surf, xc_rgb(64,64,64));
                xs_stroke_line(sys->surf, b->right-1, b->top, b->right-1, b->bottom);
                xs_stroke_line(sys->surf, b->left, b->bottom-1, b->right, b->bottom-1);

                xs_color(sys->surf, xc_rgb(128,128,128));
                xs_stroke_line(sys->surf, b->right-2, b->top+1, b->right-2, b->bottom-1);
                xs_stroke_line(sys->surf, b->left+1,b->bottom-2, b->right-2, b->bottom-2);
            } else {
                xs_color(sys->surf, xc_rgb(0,0,0));
                xs_stroke_rect(sys->surf, b->left, b->top, b->width-1, b->height-1, 0);
                xs_color(sys->surf, xc_rgb(135,136,143));
                xs_stroke_rect(sys->surf, b->left+1, b->top+1, b->width-3, b->height-3, 0);
            }
            if (btn->state == UI_FOCUSED) {
                xs_line_thickness(sys->surf, 1);
                xs_line_style(sys->surf, XLINE_DASHED);
                xs_color(sys->surf, xc_rgb(0,0,0));
                xs_stroke_rect(sys->surf, b->left+3, b->top+3, b->width-7, b->height-7, 0);
            }
        } break;}
    }
}
static void
ui_button_end(struct ui_ctx *ctx, struct ui_panel *btn)
{
    ui_panel_end(btn);
}

/* ---------------------------------------------------------------------------
 *                                  Icon
 * --------------------------------------------------------------------------- */
static void
ui_icon(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        const struct xsurface *xs = res_icon(ctx->res);
        if (xs) xs_blit(sys->surf, xs, pan->box.left, pan->box.top, 0,0, xs->w, xs->h);
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  Label
 * --------------------------------------------------------------------------- */
static void
ui_label(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent,
    const char *str_begin, const char *str_end)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        const struct xfont *xf = res_font(ctx->res);
        const char *end = !str_end ? str_begin + strlen(str_begin): str_end;
        struct ui_text_bounds bounds = ui_text_fit(pan->box.width, xf, str_begin, end);

        switch (pan->state) {
        default: assert(0); break;
        case UI_DISABLED: {
            xs_color(sys->surf, xc_rgb(255,255,255));
            xs_draw_text(sys->surf, pan->box.left+1, pan->box.top+1,
                pan->box.width, pan->box.height, str_begin, bounds.len);
            xs_color(sys->surf, xc_rgb(153,153,153));
            xs_draw_text(sys->surf, pan->box.left, pan->box.top,
                pan->box.width, pan->box.height, str_begin, bounds.len);
        } break;
        case UI_FOCUSED:
        case UI_NORMAL: {
            xs_draw_text(sys->surf, pan->box.left, pan->box.top,
                pan->box.width, pan->box.height, str_begin, bounds.len);
        } break;}
    } ui_panel_end(pan);
}

static void
ui_labelf(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent,
    const char* fmt, ...)
{
    int n = 0;
    char buf[2*1024];

    va_list args;
    va_start(args, fmt);
    n = vsnprintf(buf, cntof(buf), fmt, args);
    va_end(args);

    if (n >= cntof(buf)) return;
    ui_label(ctx, pan, parent, buf, 0);
}

/* ---------------------------------------------------------------------------
 *                                  Time
 * --------------------------------------------------------------------------- */
static void
ui_time(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent,
    const char *fmt, struct tm* time)
{
    char buf[2*1024];
    strftime(buf, cntof(buf), fmt, time);
    ui_label(ctx, pan, parent, buf, 0);
}

/* ---------------------------------------------------------------------------
 *                                  Check
 * --------------------------------------------------------------------------- */
#define UI_CHECKBOX_SIZE 13

static ui_bool
ui_check(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent,
    ui_bool checked)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        const struct ui_box *b = &pan->box;
        if (pan->clicked)
            checked = !checked;

        /* draw background */
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_line_thickness(sys->surf, 1);
        if (pan->state == UI_DISABLED)
            xs_color(sys->surf, xc_rgb(212,208,200));
        else xs_color(sys->surf, xc_rgb(255,255,255));
        xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom-1);
        xs_stroke_line(sys->surf, b->left, b->top, b->right-1, b->top);

        xs_color(sys->surf, xc_rgb(64,64,64));
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->right-2, b->top+1);
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->left+1, b->bottom-2);

        if (pan->state == UI_DISABLED)
            xs_color(sys->surf, xc_rgb(255,255,255));
        else xs_color(sys->surf, xc_rgb(212,208,200));
        xs_stroke_line(sys->surf, b->right-2, b->top+1, b->right-2, b->bottom-1);
        xs_stroke_line(sys->surf, b->left+1, b->bottom-2, b->right-2, b->bottom-2);

        if (checked) {
            /* draw cursor */
            const struct xsurface *icon = 0;
            if (pan->state == UI_DISABLED)
                icon = res_img(ctx->res, IMG_CHECK_DISABLED);
            else icon = res_img(ctx->res, IMG_CHECK);
            xs_blit(sys->surf, icon, b->left + 3, b->top + 3, 0,0, icon->w, icon->h);
        }
    }
    ui_panel_end(pan);
    return checked;
}
/* ---------------------------------------------------------------------------
 *                                  Checkbox
 * --------------------------------------------------------------------------- */
static int
ui_checkbox(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, int *checked, const char *txt, const char *end)
{
    const int old = *checked;
    pan->focusable = ui_true;
    ui_panel_begin(ctx, pan, parent); {
        static const int pad = 4;
        if (pan->clicked)
            *checked = !*checked;

        /* Check */
        struct ui_panel chk = {0};
        {struct ui_panel slot = {0};
        slot.unselectable = ui_true;
        ui_anchor_left_right(&slot.box, pan->box.left + pad, pan->box.right - pad);
        ui_anchor_top_height(&slot.box, pan->box.top, pan->box.height);
        ui_panel_begin(ctx, &slot, pan); {
            const struct xfont *xf = res_font(ctx->res);
            ui_anchor_left_width(&chk.box, slot.box.left, xf->h);
            ui_anchor_center_height(&chk.box, slot.box.center_y, xf->h);
            ui_check(ctx, &chk, &slot, *checked);
        } ui_panel_end(&slot);}

        /* Label */
        {struct ui_panel slot = {0};
        ui_anchor_left_right(&slot.box, chk.box.right + pad, pan->box.right - pad);
        ui_anchor_top_height(&slot.box, pan->box.top, pan->box.height);
        ui_panel_begin(ctx, &slot, pan); {
            const struct sys *sys = ctx->sys;
            xs_color(sys->surf, xc_rgb(0,0,0));

            /* draw label */
            struct ui_panel lbl = {0};
            struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
            ui_anchor_left_width(&lbl.box, slot.box.left, ext.w);
            ui_anchor_center_height(&lbl.box, slot.box.center_y, ext.h);
            ui_label(ctx, &lbl, &slot, txt, end);

            /* layouting */
            switch (ctx->layout) {
            case UI_LAYOUT_STRETCH: default: break;
            case UI_LAYOUT_FIT: {
                int box_right = min(lbl.box.right + pad, pan->box.right + pad);
                ui_anchor_left_right(&pan->box, pan->box.left, box_right);
            } break;}
        } ui_panel_end(&slot);}

        /* draw focus selection */
        if (pan->state == UI_FOCUSED) {
            struct sys *sys = ctx->sys;
            struct ui_box *b = &pan->box;
            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_DASHED);
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_rect(sys->surf, b->left+3, b->top+3, b->width-7, b->height-7, 0);
        }
    } ui_panel_end(pan);
    return old != *checked;
}

/* ---------------------------------------------------------------------------
 *                                  Group Box
 * --------------------------------------------------------------------------- */
static void
ui_group_box_begin(struct ui_ctx *ctx, struct ui_group_box *grp,
    struct ui_panel *pan, struct ui_panel *parent,
    const char *txt, const char *end)
{
    /* setup group panel */
    const struct sys *sys = ctx->sys;
    static const int padx = 4, pady = 4;
    static const int title_pad = 8;
    grp->pan = pan;

    ui_anchor_left_right(&pan->box, parent->box.left + padx, parent->box.right - padx);
    ui_anchor_top_bottom(&pan->box, parent->box.top, parent->box.bottom - pady);

    /* measure and draw group title */
    end = !end ? txt + strlen(txt): end;
    struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
    pan->box.top += (ext.h >> 1);
    grp->title_right = title_pad + ext.w + 1;
    grp->title_pad = title_pad;

    xs_color(sys->surf, xc_rgb(0,0,0));
    xs_draw_text(sys->surf, pan->box.left + title_pad, pan->box.top,
        ext.w, ext.h, txt, cast(int,end-txt));

    ui_anchor_top_bottom(&pan->box, parent->box.top + ext.h, parent->box.bottom - pady);
    ui_panel_begin(ctx, pan, parent);
}
static void
ui_group_box_end(struct ui_ctx *ctx, struct ui_group_box *grp)
{
    static const int padx = 4, pady = 4;
    const struct sys *sys = ctx->sys;
    struct ui_panel *pan = grp->pan;

    if (grp->flow & UI_GROUP_BOX_FIT_WIDTH)
        ui_anchor_left_width(&pan->box, pan->box.left, min(pan->box.width, pan->max_x - pan->box.left + padx));
    if (grp->flow & UI_GROUP_BOX_FIT_HEIGHT)
        ui_anchor_top_height(&pan->box, pan->box.top, min(pan->box.height, pan->max_y - pan->box.top + pady));
    ui_panel_end(pan);

    grp->title_right += pan->box.left;
    grp->top += pan->box.top;
    grp->title_pad += pan->box.left-1;

    /* draw group border */
    xs_line_thickness(sys->surf, 1);
    xs_line_style(sys->surf, XLINE_SOLID);

    const struct ui_box *b = &pan->box;
    xs_color(sys->surf, xc_rgb(128,128,128));
    xs_stroke_line(sys->surf, b->left, grp->top, grp->title_pad, grp->top);
    xs_stroke_line(sys->surf, grp->title_right, grp->top, b->right, grp->top);
    xs_stroke_line(sys->surf, b->left, grp->top, b->left, b->bottom-1);
    xs_stroke_line(sys->surf, b->left, b->bottom-2, b->right-1, b->bottom-2);
    xs_stroke_line(sys->surf, b->right-2, grp->top, b->right-2, b->bottom-1);

    xs_color(sys->surf, xc_rgb(255,255,255));
    xs_stroke_line(sys->surf, b->left+1, grp->top+1, grp->title_pad, grp->top+1);
    xs_stroke_line(sys->surf, grp->title_right, grp->top+1, b->right-2, grp->top+1);
    xs_stroke_line(sys->surf, b->left+1, grp->top+1, b->left+1, b->bottom-2);
    xs_stroke_line(sys->surf, b->left, b->bottom-1, b->right, b->bottom-1);
    xs_stroke_line(sys->surf, b->right-1, grp->top, b->right-1, b->bottom-1);
}

/* ---------------------------------------------------------------------------
 *                                  Radio-Group
 * --------------------------------------------------------------------------- */
static int
ui_radio_symbol(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, const char *txt, const char *end, int active)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        if (pan->clicked) active = !active;

        /* draw background */
        if (pan->state == UI_DISABLED)
            xs_color(sys->surf, xc_rgb(212,208,200));
        else xs_color(sys->surf, xc_rgb(255,255,255));
        xs_fill_circle(sys->surf, pan->box.left, pan->box.top,
            pan->box.width, pan->box.height);

        xs_line_thickness(sys->surf, 2);
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_circle(sys->surf, pan->box.left, pan->box.top,
            pan->box.width, pan->box.height);

        /* draw cursor */
        if (active) {
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_fill_circle(sys->surf, pan->box.left+3, pan->box.top+3,
                pan->box.width-6, pan->box.height-6);
        }
    } ui_panel_end(pan);
    return active;
}
static int
ui_radio(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, const char *txt, const char *end, int active)
{
    const int old = active;
    pan->focusable = ui_true;
    ui_panel_begin(ctx, pan, parent); {
        static const int pad = 4;
        const struct xfont *xf = res_font(ctx->res);
        if (pan->clicked) active = !active;

        /* radio symbol */
        struct ui_panel sym = {0};
        sym.unselectable = ui_true;
        ui_anchor_left_width(&sym.box, pan->box.left + pad, xf->h);
        ui_anchor_center_height(&sym.box, pan->box.center_y, xf->h);
        ui_radio_symbol(ctx, &sym, pan, txt, end, active);

        /* Label */
        {struct ui_panel slot = {0};
        ui_anchor_left_right(&slot.box, sym.box.right + pad, pan->box.right - pad);
        ui_anchor_top_height(&slot.box, pan->box.top, pan->box.height);
        ui_panel_begin(ctx, &slot, pan); {
            const struct sys *sys = ctx->sys;
            xs_color(sys->surf, xc_rgb(0,0,0));

            /* draw label */
            struct ui_panel lbl = {0};
            struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
            ui_anchor_left_width(&lbl.box, slot.box.left, ext.w);
            ui_anchor_center_height(&lbl.box, slot.box.center_y, ext.h);
            ui_label(ctx, &lbl, &slot, txt, end);

            /* layouting */
            switch (ctx->layout) {
            case UI_LAYOUT_STRETCH: default: break;
            case UI_LAYOUT_FIT: {
                int box_right = min(lbl.box.right + pad, pan->box.right);
                ui_anchor_left_right(&pan->box, pan->box.left, box_right);
            } break;}
        } ui_panel_end(&slot);}

        /* draw focus */
        if (pan->state == UI_FOCUSED) {
            struct sys *sys = ctx->sys;
            struct ui_box *b = &pan->box;
            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_DASHED);
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_rect(sys->surf, b->left+3, b->top+3, b->width-7, b->height-7, 0);
        }
    } ui_panel_end(pan);
    return old != active && active;
}
static int
ui_radio_group(struct ui_ctx *ctx,
    struct ui_radio_group *grp, struct ui_panel *parent)
{
    if (grp->state == UI_CONSISTENT) return 0;
    grp->state = UI_CONSISTENT;
    ui_record_store(&grp->record, ctx, parent);
    grp->idx = 0;
    return 1;
}
static void
ui_radio_button(struct ui_ctx *ctx, struct ui_radio_group *grp,
    struct ui_panel *pan, struct ui_panel *parent, const char *txt, const char *end)
{
    if (ui_radio(ctx, pan, parent, txt, end, grp->selected == grp->idx)) {
        grp->state = UI_INCONSISTENT;
        grp->selected = grp->idx;
        grp->toggled = ui_true;
    } grp->idx++;
}

/* ---------------------------------------------------------------------------
 *                                  Button-Label
 * --------------------------------------------------------------------------- */
static void
ui_button_label(struct ui_ctx *ctx, struct ui_panel *btn, struct ui_panel *parent,
    const char *txt, const char *end)
{
    /* Layouting */
    static const int pad = 4;
    struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
    switch (ctx->layout) {
    case UI_LAYOUT_STRETCH: default: break;
    case UI_LAYOUT_FIT: {
        int btn_width = min(btn->box.width, ext.w + 2 * pad);
        ui_anchor_left_width(&btn->box, btn->box.left, btn_width);
    } break;}

    /* Widget */
    btn->focusable = ui_true;
    ui_button_begin(ctx, btn, parent); {
        struct ui_panel lbl = {0};
        lbl.unselectable = ui_true;
        ui_anchor_center_width(&lbl.box, btn->box.center_x, ext.w);
        ui_anchor_center_height(&lbl.box, btn->box.center_y, ext.h);
        ui_label(ctx, &lbl, btn, txt, end);
    } ui_button_end(ctx, btn);
}

/* ---------------------------------------------------------------------------
 *                                  Button-Icon
 * --------------------------------------------------------------------------- */
static void
ui_button_icon(struct ui_ctx *ctx, struct ui_panel *btn, struct ui_panel *parent)
{
    btn->focusable = ui_true;
    ui_button_begin(ctx, btn, parent); {
        const struct xsurface *xs = res_icon(ctx->res);
        struct ui_panel ico = {0};
        ico.unselectable = ui_true;
        ui_anchor_center_width(&ico.box, btn->box.center_x, xs->w);
        ui_anchor_center_height(&ico.box, btn->box.center_y, xs->h);
        ui_icon(ctx, &ico, btn);
    } ui_button_end(ctx, btn);
}

/* ---------------------------------------------------------------------------
 *                              Icon-Label
 * --------------------------------------------------------------------------- */
static void
ui_icon_label(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, const char *txt, const char *end)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        const struct xsurface *xs = res_icon(ctx->res);
        struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);

        /* icon */
        struct ui_panel ico = {0};
        ico.unselectable = ui_true;
        ui_anchor_left_width(&ico.box, pan->box.left + 4, xs->w);
        ui_anchor_center_height(&ico.box, pan->box.center_y, xs->h);
        ui_icon(ctx, &ico, pan);

        /* label */
        struct ui_panel lbl = {0};
        lbl.unselectable = ui_true;
        xs_color(sys->surf, xc_rgb(0,0,0));
        ui_anchor_left_width(&lbl.box, ico.box.right + 4, ext.w);
        ui_anchor_center_height(&lbl.box, pan->box.center_y, ext.h);
        ui_label(ctx, &lbl, parent, txt, end);
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                              Button-Icon-Label
 * --------------------------------------------------------------------------- */
static void
ui_button_icon_label(struct ui_ctx *ctx, struct ui_panel *btn,
    struct ui_panel *parent, const char *txt, const char *end)
{
    static const int pad = 4;
    static const int spacing = 4;
    const struct xsurface *xs = res_icon(ctx->res);
    struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);

    /* Layouting */
    switch (ctx->layout) {
    case UI_LAYOUT_STRETCH: default: break;
    case UI_LAYOUT_FIT: {
        const int btn_w = min(btn->box.width, ext.w + 2*pad + spacing + xs->w);
        const int btn_h = max(btn->box.height, ext.h + 2*pad);
        ui_anchor_left_width(&btn->box, btn->box.left, btn_w);
        ui_anchor_top_height(&btn->box, btn->box.top, btn_h);
    } break;}

    /* Widget */
    btn->focusable = ui_true;
    ui_button_begin(ctx, btn, parent); {
        struct ui_panel pan = {.box = btn->box};
        pan.unselectable = ui_true;
        ui_icon_label(ctx, &pan, btn, txt, end);
    } ui_button_end(ctx, btn);
}

/* ---------------------------------------------------------------------------
 *                              Desktop-Icon
 * --------------------------------------------------------------------------- */
static void
ui_desktop_icon(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, const char *txt, const char *end)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct xsurface *xs = res_icon(ctx->res);
        static const int pad = 4;

        /* Icon */
        struct ui_panel ico = {0};
        ico.unselectable = ui_true;
        ui_anchor_center_width(&ico.box, pan->box.center_x, xs->w);
        ui_anchor_top_height(&ico.box, pan->box.top + pad, xs->h);
        ui_icon(ctx, &ico, pan);

        /* align */
        struct ui_panel slot = {0};
        slot.unselectable = ui_true;
        ui_anchor_top_bottom(&slot.box, ico.box.bottom + 2, pan->box.bottom-4);
        ui_anchor_left_right(&slot.box, pan->box.left, pan->box.right);
        ui_panel_begin(ctx, &slot, pan); {
            struct ui_text_bounds b;
            struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
            b = ui_text_fit(slot.box.width - pad, res_font(ctx->res), txt, end);

            /* Label */
            struct ui_panel lbl = {0};
            lbl.unselectable = ui_true;
            ui_anchor_center_width(&lbl.box, slot.box.center_x, b.width);
            ui_anchor_center_height(&lbl.box, slot.box.center_y, ext.h);
            xs_color(ctx->sys->surf, xc_rgb(0,0,0));
            ui_label(ctx, &lbl, &slot, txt, end);

        } ui_panel_end(&slot);
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                              Tool-Button
 * --------------------------------------------------------------------------- */
static void
ui_tool_button(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, const char *txt, const char *end)
{
    pan->focusable = ui_true;
    ui_button_begin(ctx, pan, parent); {
        struct ui_panel ico = {.box = pan->box};
        ico.unselectable = ui_true;
        ui_desktop_icon(ctx, &ico, pan, txt, end);
    } ui_button_end(ctx, pan);
}

/* ---------------------------------------------------------------------------
 *                                  Scroll
 * --------------------------------------------------------------------------- */
static void
ui_scroll_cursor_layout(struct ui_panel *cur, struct ui_scroll *s)
{
    /* Calculate cursor bounds */
    struct ui_box *p = &s->pan.box;
    int x = floori((float)p->left + (s->off_x / s->total_x) * (float)p->width);
    int y = floori((float)p->top + (s->off_y / s->total_y) * (float)p->height);
    int w = ceili((s->size_x / s->total_x) * (float)p->width);
    int h = ceili((s->size_y / s->total_y) * (float)p->height);

    ui_anchor_left_width(&cur->box, x, w);
    ui_anchor_top_height(&cur->box, y, h);
}
static void
ui_scroll_cursor(struct ui_ctx *ctx, struct ui_scroll *s,
    struct ui_panel *pan, struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, &s->pan); {
        const struct sys *sys = ctx->sys;

        /* Input */
        if (pan->dragged) {
            const struct ui_box *b = &parent->box;
            const int scrl_x = (pan->box.left + sys->mouse.d.x) - b->left;
            const int scrl_y = (pan->box.top + sys->mouse.d.y) - b->top;
            const float scrl_rx = cast(float, scrl_x) / cast(float, b->width);
            const float scrl_ry = cast(float, scrl_y) / cast(float, b->height);

            /* validate and update cursor position */
            s->off_x = clamp(0, scrl_rx * s->total_x, s->total_x - s->size_x);
            s->off_y = clamp(0, scrl_ry * s->total_y, s->total_y - s->size_y);
            ui_scroll_cursor_layout(pan, s);
            s->scrolled = ui_true;
        }
        /* Draw */
        const struct ui_box *c = &pan->box;
        switch (pan->state) {
        default: assert(0); break;
        case UI_DISABLED: break;
        case UI_FOCUSED:
        case UI_NORMAL: {
            if (!pan->down) {
                xs_line_style(sys->surf, XLINE_SOLID);
                xs_color(sys->surf, xc_rgb(212,208,200));
                xs_fill_rect(sys->surf, c->left, c->top, c->width, c->height, 0);

                xs_color(sys->surf, xc_rgb(212,208,200));
                xs_stroke_line(sys->surf, c->left, c->top, c->right-1, c->top);
                xs_stroke_line(sys->surf, c->left, c->top, c->left, c->bottom-1);

                xs_color(sys->surf, xc_rgb(255,255,255));
                xs_stroke_line(sys->surf, c->left+1, c->top+1, c->right-2, c->top+1);
                xs_stroke_line(sys->surf, c->left+1, c->top+1, c->left+1, c->bottom-2);

                xs_color(sys->surf, xc_rgb(64,64,64));
                xs_stroke_line(sys->surf, c->right-1, c->top, c->right-1, c->bottom);
                xs_stroke_line(sys->surf, c->left, c->bottom-1, c->right, c->bottom-1);

                xs_color(sys->surf, xc_rgb(128,128,128));
                xs_stroke_line(sys->surf, c->right-2, c->top+1, c->right-2, c->bottom-1);
                xs_stroke_line(sys->surf, c->left+1,c->bottom-2, c->right-2, c->bottom-2);
            } else {
                xs_color(sys->surf, xc_rgb(212,208,200));
                xs_fill_rect(sys->surf, c->left+1, c->top+1, c->width-3, c->height-3, 0);

                xs_line_style(sys->surf, XLINE_SOLID);
                xs_color(sys->surf, xc_rgb(0,0,0));
                xs_stroke_rect(sys->surf, c->left, c->top, c->width-1, c->height-1, 0);
            }
        } break;}
    } ui_panel_end(pan);
}
static void
ui_scroll(struct ui_ctx *ctx, struct ui_scroll *s, struct ui_panel *parent)
{
    /* Validate scroll values */
    struct ui_panel *p = &s->pan;
    ui_panel_begin(ctx, p, parent); {
        s->size_x = max(s->size_x, 1);
        s->size_y = max(s->size_y, 1);
        s->total_x = max(s->total_x, 1);
        s->total_y = max(s->total_y, 1);
        s->total_x = max(s->total_x, s->size_x);
        s->total_y = max(s->total_y, s->size_y);
        s->off_x = clamp(0, s->off_x, s->total_x - s->size_x);
        s->off_y = clamp(0, s->off_y, s->total_y - s->size_y);

        /* draw background */
        const struct ui_box *b = &p->box;
        switch (p->state) {
        default: assert(0); break;
        case UI_DISABLED: break;
        case UI_FOCUSED:
        case UI_NORMAL: {
            const struct sys *sys = ctx->sys;
            xs_color(sys->surf, xc_rgb(255,255,255));
            xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);
            xs_line_thickness(sys->surf, 1);
            xs_color(sys->surf, xc_rgb(212,208,200));
            xs_line_style(sys->surf, XLINE_DASHED);
            for (int i = 0; i < b->width; i++)
                xs_stroke_line(sys->surf, b->left+i, b->top+(i&1), b->left+i, b->bottom);
        } break;}

        /* Cursor */
        struct ui_panel cur = {0};
        ui_scroll_cursor_layout(&cur, s);
        ui_scroll_cursor(ctx, s, &cur, p);
    } ui_panel_end(p);
}

/* ---------------------------------------------------------------------------
 *                                  Arrow
 * --------------------------------------------------------------------------- */
enum ui_direction {
    UI_NORTH = 0x01,
    UI_WEST = 0x02,
    UI_SOUTH = 0x04,
    UI_EAST = 0x08
};
enum ui_dir {
    UI_DIR_HORIZONTAL = UI_NORTH|UI_SOUTH,
    UI_DIR_VERTICAL = UI_WEST|UI_EAST,
};
static void
ui_arrow(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent,
    enum ui_direction orient)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        if (pan->state == UI_DISABLED)
            xs_color(sys->surf, xc_rgb(152,148,140));
        else xs_color(sys->surf, xc_rgb(0,0,0));
        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_SOLID);

        switch (orient) {
        default: assert(0); break;
        case UI_NORTH: {
            for (int i = 0; i < pan->box.height; ++i)
                xs_stroke_line(sys->surf, pan->box.left+1+i, pan->box.bottom-1-i,
                    pan->box.left+1+pan->box.width-i, pan->box.bottom-1-i);
        } break;
        case UI_WEST: {
            for (int i = 0; i < pan->box.width; ++i)
                xs_stroke_line(sys->surf, pan->box.right-2-i, pan->box.top-1+i,
                    pan->box.right-2-i, pan->box.top+pan->box.height+1-i);
        } break;
        case UI_SOUTH: {
            for (int i = 0; i < pan->box.height; ++i)
                xs_stroke_line(sys->surf, pan->box.left+1+i, pan->box.top+i,
                    pan->box.left+1+pan->box.width-i, pan->box.top+i);
        } break;
        case UI_EAST: {
            for (int i = 0; i < pan->box.width; ++i)
                xs_stroke_line(sys->surf, pan->box.left+2+i, pan->box.top-1+i,
                    pan->box.left+2+i, pan->box.top+pan->box.height+1-i);
        } break;}
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  Scrollbar
 * --------------------------------------------------------------------------- */
static void
ui_scrollbar_button(struct ui_ctx *ctx, struct ui_panel *btn,
    struct ui_panel *parent, enum ui_direction orient)
{
    ui_button_begin(ctx, btn, parent); {
        const struct sys *sys = ctx->sys;
        xs_color_background(sys->surf, xc_rgb(0,0,0));
        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_SOLID);

        /* draw button */
        struct ui_box *b = &btn->box;
        xs_color(sys->surf, xc_rgb(212,208,200));
        xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);
        xs_stroke_line(sys->surf, b->left, b->top, b->right-1, b->top);
        xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom-1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->right-2, b->top+1);
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->left+1, b->bottom-2);

        xs_color(sys->surf, xc_rgb(64,64,64));
        xs_stroke_line(sys->surf, b->right-1, b->top, b->right-1, b->bottom);
        xs_stroke_line(sys->surf, b->left, b->bottom-1, b->right, b->bottom-1);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, b->right-2, b->top+1, b->right-2, b->bottom-1);
        xs_stroke_line(sys->surf, b->left+1,b->bottom-2, b->right-2, b->bottom-2);

        /* arrow icon */
        static const int arrow_w = 5;
        static const int arrow_h = 3;

        struct ui_panel sym = {0};
        ui_anchor_center_width(&sym.box, btn->box.center_x, arrow_w);
        ui_anchor_center_height(&sym.box, btn->box.center_y, arrow_h);
        ui_arrow(ctx, &sym, btn, orient);
    } ui_button_end(ctx, btn);
}
static void
ui_scrollbarh(struct ui_ctx *ctx, struct ui_scrollbar *s,
    struct ui_panel *pan, struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, parent);{
        s->scrolled = ui_false;
        /* decrement button */
        struct ui_panel btn_dec = {0};
        ui_anchor_left_width(&btn_dec.box, pan->box.left, pan->box.height);
        ui_anchor_bottom_height(&btn_dec.box, pan->box.bottom, pan->box.height);
        ui_scrollbar_button(ctx, &btn_dec, pan, UI_WEST);
        if (btn_dec.pressed) {
            s->off -= s->size * 0.1f;
            s->scrolled = ui_true;
            sys_clear(ctx->sys);
        }
        /* increment button */
        struct ui_panel btn_inc = {0};
        ui_anchor_right_width(&btn_inc.box, pan->box.right, pan->box.height);
        ui_anchor_bottom_height(&btn_inc.box, pan->box.bottom, pan->box.height);
        ui_scrollbar_button(ctx, &btn_inc, pan, UI_EAST);
        if (btn_inc.pressed) {
            s->off += s->size * 0.1f;
            s->scrolled = ui_true;
            sys_clear(ctx->sys);
        }
        /* scroll */
        struct ui_scroll scrl;
        scrl.total_x = s->total;
        scrl.total_y = scrl.size_y = 1;
        scrl.size_x = s->size;
        scrl.off_x = s->off;
        scrl.off_y = 0;

        ui_anchor_bottom_height(&scrl.pan.box, pan->box.bottom, pan->box.height);
        ui_anchor_left_right(&scrl.pan.box, btn_dec.box.right, btn_inc.box.left);
        ui_scroll(ctx, &scrl, pan);

        s->scrolled = s->scrolled || scrl.scrolled;
        s->off = scrl.off_x;
    } ui_panel_end(pan);
}
static void
ui_scrollbarv(struct ui_ctx *ctx, struct ui_scrollbar *s,
    struct ui_panel *pan, struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, parent); {
        s->scrolled = ui_false;
        /* decrement button */
        struct ui_panel btn_dec = {0};
        ui_anchor_left_width(&btn_dec.box, pan->box.left, pan->box.width);
        ui_anchor_top_height(&btn_dec.box, pan->box.top, pan->box.width);
        ui_scrollbar_button(ctx, &btn_dec, pan, UI_NORTH);
        if (btn_dec.pressed) {
            s->off -= s->size * 0.1f;
            s->scrolled = ui_true;
            sys_clear(ctx->sys);
        }
        /* increment button */
        struct ui_panel btn_inc = {0};
        ui_anchor_left_width(&btn_inc.box, pan->box.left, pan->box.width);
        ui_anchor_bottom_height(&btn_inc.box, pan->box.bottom, pan->box.width);
        ui_scrollbar_button(ctx, &btn_inc, pan, UI_SOUTH);
        if (btn_inc.pressed) {
            s->off += s->size * 0.1f;
            s->scrolled = ui_true;
            sys_clear(ctx->sys);
        }
        /* scroll */
        struct ui_scroll scrl = {0};
        scrl.total_x = scrl.size_x = 1;
        scrl.total_y = s->total;
        scrl.size_y = s->size;
        scrl.off_y = s->off;
        scrl.off_x = 0;

        ui_anchor_top_bottom(&scrl.pan.box, btn_dec.box.bottom, btn_inc.box.top);
        ui_anchor_left_right(&scrl.pan.box, pan->box.left, pan->box.right);
        ui_scroll(ctx, &scrl, pan);

        s->scrolled = s->scrolled || scrl.scrolled;
        s->off = scrl.off_y;
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  Text Box
 * --------------------------------------------------------------------------- */
static int ui_rune_is_default(long c) {return ui_true;}
static int ui_rune_is_binary(long c) {return c == '0' || c == '1';}

static int
ui_rune_is_numeric(long c)
{
    return (c >= '0' && c <= '9') || (c == '+') || (c == '-');
}
static int
ui_rune_is_real(long c)
{
    return (c >= '0' && c <= '9') || (c == '+') || (c == '-') || (c == '.');
}
static int
ui_rune_is_hex(long c)
{
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
static void
ui_edit_box_input_text(struct ui_ctx *ctx, struct ui_edit_box *box,
    ui_filter_f rune_filter, char *buf, int *len, int max)
{
    struct utf8_iter it;
    struct sys *sys = ctx->sys;
    for (utf8_begin(&it, sys->text, sys->text + sys->text_len); utf8_next(&it);) {
        if (rune_filter(it.rune) == ui_false) continue;
        if (*len + it.rune_len >= max) break;

        memcpy(buf + *len, it.rune_begin, cast(size_t, it.rune_len));
        *len += it.rune_len;
    } sys->text_len = 0;
}
static void
ui_edit_box_input(struct ui_ctx *ctx, struct ui_edit_box *box,
    char *buf, int *len, int max)
{
    struct sys *sys = ctx->sys;
    if (sys->text_len) {
        static const ui_filter_f filter[UI_EDIT_BOX_MAX] = {
            ui_rune_is_default,
            ui_rune_is_numeric,
            ui_rune_is_real,
            ui_rune_is_hex,
            ui_rune_is_binary
        };
        if (box->filter)
            ui_edit_box_input_text(ctx, box, box->filter, buf, len, max);
        else ui_edit_box_input_text(ctx, box, filter[box->type], buf, len, max);
    } else if (sys->keys[SYS_KEY_BACKSPACE].pressed)
        *len = max(0, *len - 1);
}
static void
ui_edit_field(struct ui_ctx *ctx, struct ui_edit_box *box,
    struct ui_panel *pan, struct ui_panel *parent,
    char *buf, int *len, int max)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        /* text input */
        if (pan->pressed) ctx->active = pan->id;
        const int active = ui_id_eq(&pan->id, &ctx->active);
        if (active) ui_edit_box_input(ctx, box, buf, len, max);

        /* calculate text measurements */
        struct ui_text_window_bounds bounds = {0};
        const struct xfont *xf = res_font(ctx->res);
        struct extend ext = xf_text_measure(xf, buf, buf+*len);
        if (ext.w <= pan->box.width) {
            bounds.width = ext.w;
            bounds.len = *len;
            bounds.off = 0;
        } else bounds = ui_text_fit_window(pan->box.width, xf, buf, *len);

        /* store temporary state  */
        const unsigned fg = xs_color_foreground(sys->surf, xc_rgb(0,0,0));
        const unsigned bg = xs_color_background(sys->surf, xc_rgb(255,255,255));

        /* draw text  */
        struct ui_panel lbl = {0};
        ui_anchor_left_right(&lbl.box, pan->box.left, pan->box.right);
        ui_anchor_center_height(&lbl.box, pan->box.center_y, ext.h);
        ui_label(ctx, &lbl, pan, buf + bounds.off, buf + *len);

        if (active) {
            /* draw cursor */
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_line(sys->surf, pan->box.left + bounds.width,
                pan->box.top+1, pan->box.left + bounds.width,
                pan->box.bottom);
        }
        /* draw focus */
        if (pan->state == UI_FOCUSED) {
            xs_line_style(sys->surf, XLINE_DASHED);
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_rect(sys->surf, pan->box.left-1, pan->box.top+1,
                pan->box.width, pan->box.height-1, 0);
        }
        /* restore state  */
        xs_color_foreground(sys->surf, fg);
        xs_color_background(sys->surf, bg);
    }
    ui_panel_end(pan);
}
static void
ui_edit_box(struct ui_ctx *ctx, struct ui_edit_box *box, struct ui_panel *pan,
    struct ui_panel *parent, char *buf, int *len, int max)
{
    const struct sys *sys = ctx->sys;
    ui_panel_begin(ctx, pan, parent); {
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_line_thickness(sys->surf, 1);

        /* draw background */
        if (pan->state == UI_DISABLED)
            xs_color(sys->surf, xc_rgb(212,208,200));
        else xs_color(sys->surf, xc_rgb(255,255,255));
        xs_fill_rect(sys->surf, pan->box.left, pan->box.top, pan->box.width, pan->box.height, 0);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, pan->box.left, pan->box.top, pan->box.right, pan->box.top);
        xs_stroke_line(sys->surf, pan->box.left, pan->box.top, pan->box.left, pan->box.bottom);

        xs_color(sys->surf, xc_rgb(212,208,200));
        xs_stroke_line(sys->surf, pan->box.left+1, pan->box.bottom-1, pan->box.right, pan->box.bottom-1);
        xs_stroke_line(sys->surf, pan->box.right-1, pan->box.top+1, pan->box.right-1, pan->box.bottom-1);

        xs_color(sys->surf, xc_rgb(0,0,0));
        xs_stroke_line(sys->surf, pan->box.left+1, pan->box.top+1, pan->box.right-1, pan->box.top+1);
        xs_stroke_line(sys->surf, pan->box.left+1, pan->box.top+1, pan->box.left+1, pan->box.bottom-1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, pan->box.left, pan->box.bottom, pan->box.right+1, pan->box.bottom);
        xs_stroke_line(sys->surf, pan->box.right, pan->box.top, pan->box.right, pan->box.bottom);

        /* content */
        const int padx = 4, pady = 2;
        struct ui_panel content = {0};
        content.focusable = ui_true;
        ui_anchor_left_right(&content.box, pan->box.left + padx, pan->box.right - padx);
        ui_anchor_top_bottom(&content.box, pan->box.top + pady, pan->box.bottom - pady);
        ui_edit_field(ctx, box, &content, pan, buf, len, max);
    }
    ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  Spinner
 * --------------------------------------------------------------------------- */
#define UI_SPINNER_SIZE 16

static void
ui_spinner_button(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, enum ui_direction dir)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        struct ui_box *box = &pan->box;

        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_SOLID);

        /* draw background */
        xs_color(sys->surf, xc_rgb(212,208,200));
        xs_fill_rect(sys->surf, box->left, box->top, box->width, box->height, 0);
        xs_stroke_line(sys->surf, box->left, box->top, box->right-1, box->top);
        xs_stroke_line(sys->surf, box->left, box->top, box->left, box->bottom-1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, box->left+1, box->top+1, box->right-2, box->top+1);
        xs_stroke_line(sys->surf, box->left+1, box->top+1, box->left+1, box->bottom-2);

        xs_color(sys->surf, xc_rgb(64,64,64));
        xs_stroke_line(sys->surf, box->right-1, box->top, box->right-1, box->bottom);
        xs_stroke_line(sys->surf, box->left, box->bottom-1, box->right, box->bottom-1);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, box->right-2, box->top+1, box->right-2, box->bottom-1);
        xs_stroke_line(sys->surf, box->left+1,box->bottom-2, box->right-2, box->bottom-2);

        /* draw arrow */
        static const int arrow_w = 3;
        static const int arrow_h = 2;
        struct ui_panel arrow = {0};
        arrow.unselectable = ui_true;

        ui_anchor_center_width(&arrow.box, pan->box.center_x, arrow_w);
        ui_anchor_center_height(&arrow.box, pan->box.center_y, arrow_h);
        ui_arrow(ctx, &arrow, pan, dir);
    } ui_panel_end(pan);
}
static int
ui_spin_control(struct ui_ctx *ctx, struct ui_spinner *spin,
    struct ui_panel *pan, struct ui_panel *parent, int *n)
{
    const int old_n = *n;
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;

        /* increment button */
        struct ui_panel inc_btn = {0};
        ui_anchor_left_right(&inc_btn.box, pan->box.left, pan->box.right);
        ui_anchor_top_height(&inc_btn.box, pan->box.top, pan->box.height >> 1);
        ui_spinner_button(ctx, &inc_btn, pan, UI_NORTH);
        if (inc_btn.pressed) {
            *n = min(*n + spin->inc, spin->max);
        } else if (inc_btn.dragged) {
            *n += max(sys->mouse.d.x, 0);
            *n += max(-sys->mouse.d.y, 0);
            *n = min(*n, spin->max);
        }
        /* decrement button */
        struct ui_panel dec_btn = {0};
        ui_anchor_left_right(&dec_btn.box, pan->box.left, pan->box.right);
        ui_anchor_top_height(&dec_btn.box, inc_btn.box.bottom, pan->box.height >> 1);
        ui_spinner_button(ctx, &dec_btn, pan, UI_SOUTH);
        if (dec_btn.pressed) {
            *n = max(*n - spin->inc, spin->min);
        } else if (dec_btn.dragged) {
            *n += min(sys->mouse.d.x, 0);
            *n += min(-sys->mouse.d.y, 0);
            *n = max(*n, spin->min);
        }
    } ui_panel_end(pan);
    return *n != old_n;
}
static int
ui_spinner(struct ui_ctx *ctx, struct ui_spinner *spin,
    struct ui_panel *pan, struct ui_panel *parent, int n)
{
    ui_bool loop = ui_false;
    struct ui_record record = {0};
    if ((spin->min == spin->max) && !spin->min) {
        spin->min = INT_MIN + 1;
        spin->max = INT_MAX - 1;
        spin->inc = 1;
    }
    ui_record_store(&record, ctx, pan);
    do {loop = ui_false;
        ui_record_restore(&record);
        ui_panel_begin(ctx, pan, parent); {
            const struct sys *sys = ctx->sys;

            /* draw background */
            if (pan->state == UI_DISABLED)
                xs_color(sys->surf, xc_rgb(212,208,200));
            else xs_color(sys->surf, xc_rgb(255,255,255));
            xs_fill_rect(sys->surf, pan->box.left, pan->box.top, pan->box.width, pan->box.height, 0);

            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_SOLID);

            xs_color(sys->surf, xc_rgb(128,128,128));
            xs_stroke_line(sys->surf, pan->box.left, pan->box.top, pan->box.right, pan->box.top);
            xs_stroke_line(sys->surf, pan->box.left, pan->box.top, pan->box.left, pan->box.bottom);

            xs_color(sys->surf, xc_rgb(212,208,200));
            xs_stroke_line(sys->surf, pan->box.left+1, pan->box.bottom-1, pan->box.right, pan->box.bottom-1);
            xs_stroke_line(sys->surf, pan->box.right-1, pan->box.top+1, pan->box.right-1, pan->box.bottom-1);

            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_line(sys->surf, pan->box.left+1, pan->box.top+1, pan->box.right-1, pan->box.top+1);
            xs_stroke_line(sys->surf, pan->box.left+1, pan->box.top+1, pan->box.left+1, pan->box.bottom-1);

            xs_color(sys->surf, xc_rgb(255,255,255));
            xs_stroke_line(sys->surf, pan->box.left, pan->box.bottom, pan->box.right+1, pan->box.bottom);
            xs_stroke_line(sys->surf, pan->box.right, pan->box.top, pan->box.right, pan->box.bottom);

            /* convert int to string */
            char buf[64] = "0";
            snprintf(buf, sizeof(buf), "%d", n);
            int len = cast(int, strlen(buf));

            /* Edit field */
            const int padx = 4;
            struct ui_panel edit_box = {0};
            struct ui_edit_box edit = {.type = UI_EDIT_BOX_INT};
            edit_box.focusable = ui_true;
            ui_anchor_left_right(&edit_box.box, pan->box.left + padx, pan->box.right - UI_SPINNER_SIZE - padx);
            ui_anchor_top_bottom(&edit_box.box, pan->box.top + 2, pan->box.bottom - 2);
            ui_edit_field(ctx, &edit, &edit_box, pan, buf, &len, szof(buf));

            /* convert string to int */
            if (len == 0)
                buf[0] = '0', buf[1] = 0;
            else buf[len] = 0;
            n = atoi(buf);

            /* draw focus */
            if (edit_box.state == UI_FOCUSED) {
                if (sys->keys[SYS_KEY_UP].pressed ||
                    sys->keys[SYS_KEY_RIGHT].pressed)
                    n = min(n + spin->inc, spin->max);
                if (sys->keys[SYS_KEY_DOWN].pressed ||
                    sys->keys[SYS_KEY_LEFT].pressed)
                    n = max(n - spin->inc, spin->min);
            }
            /* Buttons */
            struct ui_panel btn = {0};
            ui_anchor_right_width(&btn.box, pan->box.right, UI_SPINNER_SIZE);
            ui_anchor_top_bottom(&btn.box, pan->box.top+2, pan->box.bottom-1);
            if (ui_spin_control(ctx, spin, &btn, pan, &n)) {
                sys_clear(ctx->sys);
                loop = ui_true;
            }
        } ui_panel_end(pan);
    } while (loop);
    return n;
}

/* ---------------------------------------------------------------------------
 *                                  Area
 * --------------------------------------------------------------------------- */
#define UI_SCROLLBAR_SIZE 13

static int
ui_area_begin(struct ui_ctx *ctx, struct ui_area *d,
    struct ui_panel *pan, struct ui_panel *parent)
{
    const struct sys *sys = ctx->sys;

    if (d->state == UI_CONSISTENT) return 0;
    else d->state = UI_CONSISTENT;
    ui_record_store(&d->record, ctx, parent);

    /* calculate clip area */
    ui_anchor_left_right(&pan->box, parent->box.left + d->padx, parent->box.right - d->padx);
    ui_anchor_top_bottom(&pan->box, parent->box.top + d->pady, parent->box.bottom - d->pady);
    ui_panel_begin(ctx, pan, parent);

    /* draw background style */
    d->pan = pan;
    const struct ui_box *b = &pan->box;
    switch (d->style) {
    default: assert(0); break;
    case UI_AREA_STYLE_FLAT: break;
    case UI_AREA_STYLE_SUNKEN: {
        d->background = xs_color_background(sys->surf, xc_rgb(255,255,255));
        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);
    } break;}

    /* draw border style */
    switch (d->border_style) {
    default: assert(0); break;
    case UI_BORDER_STYLE_NONE: break;
    case UI_BORDER_STYLE_SINGLE: {
        xs_color(sys->surf, xc_rgb(0,0,0));
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_line_thickness(sys->surf, 1);
        xs_stroke_rect(sys->surf, b->left, b->top, b->width, b->height, 0);
    } break;
    case UI_BORDER_STYLE_3D: {
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_line_thickness(sys->surf, 1);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, b->left, b->top, b->right, b->top);
        xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom);

        xs_color(sys->surf, xc_rgb(0,0,0));
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->right-1, b->top+1);
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->left+1, b->bottom-1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, b->left, b->bottom, b->right, b->bottom);
        xs_stroke_line(sys->surf, b->right, b->top, b->right, b->bottom);
    } break;}

    switch (d->type) {
    default: assert(0); break;
    case UI_AREA_FIXED: break;
    case UI_AREA_SCROLL: {
        ui_anchor_left_right(&pan->box, pan->box.left, pan->box.right - UI_SCROLLBAR_SIZE);
        ui_anchor_top_bottom(&pan->box, pan->box.top, b->bottom - UI_SCROLLBAR_SIZE);
    } break;}

    d->clip_rect = ui_clip_begin(ctx, pan->box.left, pan->box.top+3,
        pan->box.width, pan->box.height);

    /* apply scroll offset to area body */
    const int off_x = floori(d->off_x);
    const int off_y = floori(d->off_y);
    ui_anchor_left_right(&pan->box, pan->box.left - off_x, pan->box.right - off_x);
    ui_anchor_top_bottom(&pan->box, pan->box.top - off_y, pan->box.bottom - off_y);
    return 1;
}
static void
ui_area_scroll(struct ui_ctx *ctx, struct ui_area *d)
{
    const int off_x = floori(d->off_x);
    const int off_y = floori(d->off_y);
    struct ui_panel *pan = d->pan;

    /* setup vertical scrollbar */
    struct ui_scrollbar vscrl = {0};
    vscrl.total = cast(float, pan->max_y - pan->box.top);
    vscrl.size = cast(float, pan->box.bottom - pan->box.top);
    vscrl.off = d->off_y;

    /* do vertical scrollbar */
    if (vscrl.total > vscrl.size) {
        struct ui_panel vpan = {0};
        int right = pan->box.right + UI_SCROLLBAR_SIZE + off_x;
        ui_anchor_left_right(&vpan.box, pan->box.right + off_x, right);
        ui_anchor_top_bottom(&vpan.box, pan->box.top+2 + off_y, pan->box.bottom + off_y);
        ui_scrollbarv(ctx, &vscrl, &vpan, d->pan);
        if (d->off_y != vscrl.off) {
            d->off_y = vscrl.off;
            d->scrolled = ui_true;
        }
    } else d->off_y = 0;

    /* setup horizontal scrollbar */
    struct ui_scrollbar hscrl = {0};
    hscrl.total = cast(float, pan->max_x - pan->box.left);
    hscrl.size = cast(float, pan->box.right - pan->box.left + UI_SCROLLBAR_SIZE);
    hscrl.off = d->off_x;

    /* do horizontal scrollbar */
    if (hscrl.total > hscrl.size) {
        struct ui_panel hpan = {0};
        int bottom = pan->box.bottom + UI_SCROLLBAR_SIZE + off_y;
        ui_anchor_left_right(&hpan.box, pan->box.left+2 + off_x, pan->box.right + off_x);
        ui_anchor_top_bottom(&hpan.box, pan->box.bottom + off_y, bottom);
        ui_scrollbarh(ctx, &hscrl, &hpan, d->pan);
        if (d->off_x != hscrl.off) {
            d->off_x = hscrl.off;
            d->scrolled = ui_true;
        }
    } else d->off_x = 0;

    d->max_offx = hscrl.total - hscrl.size;
    d->max_offy = vscrl.total - vscrl.size;
}
static void
ui_area_set_offset(struct ui_area *d, float offx, float offy)
{
    d->off_x = offx;
    d->off_y = offy;
    d->state = UI_INCONSISTENT;
}
static void
ui_area_set_offset_x(struct ui_area *d, float offx)
{
    ui_area_set_offset(d, offx, d->off_y);
}
static void
ui_area_set_offset_y(struct ui_area *d, float offy)
{
    ui_area_set_offset(d, d->off_x, offy);
}
static void
ui_area_end(struct ui_ctx *ctx, struct ui_area *d)
{
    struct sys *sys = ctx->sys;
    switch (d->style) {
    default: assert(0); break;
    case UI_AREA_STYLE_FLAT: break;
    case UI_AREA_STYLE_SUNKEN: {
        xs_color_background(sys->surf, d->background);
    } break;}

    ui_panel_end(d->pan);
    ui_clip_end(ctx, d->clip_rect);
    d->scrolled = ui_false;

    switch (d->type) {
    default: assert(0); break;
    case UI_AREA_FIXED: break;
    case UI_AREA_SCROLL: ui_area_scroll(ctx, d); break;}

    if (d->state == UI_INCONSISTENT) {
        sys_clear(sys);
        ui_record_restore(&d->record);
        memset(&d->pan, 0, sizeof(d->pan));
    }
}

/* ---------------------------------------------------------------------------
 *                                  Combo
 * --------------------------------------------------------------------------- */
#define UI_COMBO_DEFAULT_MAX_HEIGHT 400
#define UI_COMBO_SIZE 16

static void
ui_combo_button(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        struct ui_box *b = &pan->box;
        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_SOLID);

        /* draw background */
        xs_color(sys->surf, xc_rgb(212,208,200));
        xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);
        xs_stroke_line(sys->surf, b->left, b->top, b->right-1, b->top);
        xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom-1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->right-2, b->top+1);
        xs_stroke_line(sys->surf, b->left+1, b->top+1, b->left+1, b->bottom-2);

        xs_color(sys->surf, xc_rgb(64,64,64));
        xs_stroke_line(sys->surf, b->right-1, b->top, b->right-1, b->bottom);
        xs_stroke_line(sys->surf, b->left, b->bottom-1, b->right, b->bottom-1);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, b->right-2, b->top+1, b->right-2, b->bottom-1);
        xs_stroke_line(sys->surf, b->left+1, b->bottom-2, b->right-2, b->bottom-2);

        if (pan->state == UI_FOCUSED) {
            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_DASHED);
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_rect(sys->surf, b->left+2, b->top+2, b->width-5, b->height-6, 0);
        }

        /* draw arrow */
        static const int arrow_w = 7;
        static const int arrow_h = 4;
        struct ui_panel arrow = {0};
        arrow.unselectable = ui_true;

        ui_anchor_center_width(&arrow.box, pan->box.center_x, arrow_w);
        ui_anchor_center_height(&arrow.box, pan->box.center_y, arrow_h);
        ui_arrow(ctx, &arrow, pan, UI_SOUTH);
    } ui_panel_end(pan);
}
static ui_bool
ui_combo_begin(struct ui_ctx *ctx, struct ui_combo *com,
    const char *selected, const char *end,
    struct ui_panel *pan, struct ui_panel *parent)
{
    struct sys *sys = ctx->sys;
    struct ui_panel *popup = &com->popup;
    static const int pady = 4;
    com->max_height = !com->max_height ? UI_COMBO_DEFAULT_MAX_HEIGHT: com->max_height;
    com->pan = pan;

    switch (com->stage) {
    default: assert(0); break;
    case UI_COMBO_DONE:
        ui_popup_end(ctx, com->pan);
        if (!com->selection_changed)
            return ui_false;
        ui_record_restore(&com->record);
        /* fallthrough */
    case UI_COMBO_HEADER: {
        ui_record_store(&com->record, ctx, parent);
        ui_panel_begin(ctx, pan, parent); {
            /* draw combo box background */
            struct ui_box *b = &pan->box;
            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_SOLID);

            if (pan->state == UI_DISABLED)
                xs_color(sys->surf, xc_rgb(212,208,200));
            else xs_color(sys->surf, xc_rgb(255,255,255));
            xs_fill_rect(sys->surf, b->left, b->top, b->width, b->height, 0);

            xs_color(sys->surf, xc_rgb(128,128,128));
            xs_stroke_line(sys->surf, b->left, b->top, b->right, b->top);
            xs_stroke_line(sys->surf, b->left, b->top, b->left, b->bottom);

            xs_color(sys->surf, xc_rgb(212,208,200));
            xs_stroke_line(sys->surf, b->left+1, b->bottom-1, b->right, b->bottom-1);
            xs_stroke_line(sys->surf, b->right-1, b->top+1, b->right-1, b->bottom-1);

            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_line(sys->surf, b->left+1, b->top+1, b->right-1, b->top+1);
            xs_stroke_line(sys->surf, b->left+1, b->top+1, b->left+1, b->bottom-1);

            xs_color(sys->surf, xc_rgb(255,255,255));
            xs_stroke_line(sys->surf, b->left, b->bottom, b->right+1, b->bottom);
            xs_stroke_line(sys->surf, b->right, b->top, b->right, b->bottom);

            /* buffer */
            char buf[256];
            int buf_len = cast(int, strlen(selected));
            strscpy(buf, selected, cntof(buf));

            /* Edit field */
            const int padx = 4;
            ui_bool activated = ui_false;
            struct ui_panel edit_box = {0};
            struct ui_edit_box edit = {.type = UI_EDIT_BOX_DEFAULT};
            edit_box.focusable = ui_true;

            ui_anchor_left_right(&edit_box.box, pan->box.left + padx, pan->box.right - UI_COMBO_SIZE - padx);
            ui_anchor_top_bottom(&edit_box.box, pan->box.top + 2, pan->box.bottom - 2);
            ui_edit_field(ctx, &edit, &edit_box, pan, buf, &buf_len, szof(buf));

            /* button */
            struct ui_panel btn = {0};
            btn.focusable = ui_true;
            ui_anchor_right_width(&btn.box, pan->box.right, UI_COMBO_SIZE);
            ui_anchor_top_bottom(&btn.box, pan->box.top+2, pan->box.bottom-2);
            ui_combo_button(ctx, &btn, pan);
            if (btn.clicked || activated)
                ui_popup_activate(ctx, pan->id, UI_POPUP_NON_BLOCKING);
        } ui_panel_end(pan);

        if (com->stage == UI_COMBO_DONE)
            return ui_false;
        if (!ui_popup_begin(ctx, popup, pan->id, pan->box.left,
            pan->box.bottom, pan->box.width, com->max_height))
            return ui_false;

        com->state = UI_COMBO_EXPANDED;
        com->stage = UI_COMBO_LAYOUT;
    } /* fallthrough */
    case UI_COMBO_LAYOUT: {
        /* start layouting pass */
        com->idx = 0;
        com->at_y = popup->box.top + pady;
    } return ui_true;
    case UI_COMBO_EXEC: {
        /* start execution pass */
        com->idx = 0;
        com->at_y = popup->box.top + pady;
        ui_anchor_left_width(&popup->box, popup->box.left, popup->box.right-1);
        ui_area_begin(ctx, &com->area, &com->area_panel, popup);
    } return ui_true;}
    return ui_true;
}
static int
ui_combo_label(struct ui_ctx *ctx, struct ui_combo *com,
    struct ui_panel *pan, struct ui_panel *parent,
    const char *label, const char *end)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        if (pan->hovered) {
            const unsigned fg = xs_color_foreground(sys->surf, xc_rgb(0,0,0));
            const unsigned bg = xs_color_background(sys->surf, xc_rgb(255,255,255));

            /* text hightlight */
            xs_color(sys->surf, xc_rgb(0,0,128));
            xs_fill_rect(sys->surf, pan->box.left, pan->box.top,
                pan->box.width, pan->box.height, 0);
            xs_color(sys->surf, xc_rgb(255,255,255));
            ui_label(ctx, pan, parent, label, end);

            xs_color_foreground(sys->surf, fg);
            xs_color_background(sys->surf, bg);
        } else {
            xs_color(sys->surf, xc_rgb(0,0,0));
            ui_label(ctx, pan, parent, label, end);
        }
        if (pan->state == UI_FOCUSED) {
            struct ui_box *b = &pan->box;
            xs_line_thickness(sys->surf, 1);
            xs_line_style(sys->surf, XLINE_DASHED);
            xs_color(sys->surf, xc_rgb(0,0,0));
            xs_stroke_rect(sys->surf, b->left+2, b->top+2, b->width-5, b->height-6, 0);
        }
    } ui_panel_end(pan);
    return pan->clicked;
}
static int
ui_combo_item(struct ui_ctx *ctx, struct ui_combo *com,
    const char *label, const char *end)
{
    int res = 0;
    static const int padx = 4;
    static const int spacing = 4;

    switch (com->stage) {
    case UI_COMBO_HEADER:
    case UI_COMBO_DONE:
    default: assert(0); break;
    case UI_COMBO_LAYOUT: {
        /* layout */
        if (com->idx != com->selected) {
            struct extend ext = xf_text_measure(res_font(ctx->res), label,end);
            com->at_y += ext.h + spacing;
        }
    } break;
    case UI_COMBO_EXEC: {
        /* execute */
        struct ui_panel btn = {0};
        if (com->idx == com->selected) break;

        struct extend ext = xf_text_measure(res_font(ctx->res), label,end);
        ui_anchor_left_right(&btn.box, com->area_panel.box.left + padx,
            com->area_panel.box.right - padx);
        ui_anchor_top_height(&btn.box, com->at_y, ext.h);
        com->at_y += ext.h + spacing;

        if ((res = ui_combo_label(ctx, com, &btn, &com->area_panel, label, end))) {
            ui_popup_deactivate(ctx, com->pan->id);
            com->state = UI_COMBO_COLLAPSED;
            com->selection_changed = ui_true;
            com->selected = com->idx;
        }
    } break;}
    com->idx++;
    return res;
}
static void
ui_combo_end(struct ui_ctx *ctx, struct ui_combo *com)
{
    switch (com->stage) {
    case UI_COMBO_HEADER:
    case UI_COMBO_DONE:
    default: assert(0); break;
    case UI_COMBO_LAYOUT: {
        /* fit popup to content */
        static const int pady = 4;
        com->max_height = com->at_y - com->popup.box.top + pady;
        if (com->max_height < UI_COMBO_DEFAULT_MAX_HEIGHT) {
            const struct ui_panel *pan = com->pan;
            if (!ui_popup_resize(ctx, &com->popup, pan->id, pan->box.width, com->max_height)) {
                com->stage = UI_COMBO_DONE;
                return;
            }
        } com->stage = UI_COMBO_EXEC;
    } break;
    case UI_COMBO_EXEC: {
        ui_area_end(ctx, &com->area);
        if (com->area.state == UI_INCONSISTENT)
            com->stage = UI_COMBO_EXEC;
        else com->stage = UI_COMBO_DONE;
    } break;}
}
static int
ui_combo(struct ui_ctx *ctx, struct ui_combo *com,
    struct ui_panel *pan, struct ui_panel *parent,
    const char **list, int cnt)
{
    while (ui_combo_begin(ctx, com, list[com->selected], 0, pan, parent)) {
        for (int i = 0; i < cnt; ++i)
            if (ui_combo_item(ctx, com, list[i], 0)) break;
        ui_combo_end(ctx, com);
    } return com->selection_changed ? 1:0;
}

/* ---------------------------------------------------------------------------
 *                                  Tab
 * --------------------------------------------------------------------------- */
static ui_bool
ui_tab_control_begin(struct ui_ctx *ctx, struct ui_tab_control *tab,
    struct ui_panel *parent)
{
    tab->parent = parent;
    switch (tab->stage) {
    default: assert(0); break;
    case UI_TAB_LAYOUT: {
        tab->fixed_size = 0;
    } break;
    case UI_TAB_INPUT: {
        tab->off_x = parent->box.left;
        ui_record_store(&tab->rec, ctx, parent);
        tab->idx = 0;
    } break;
    case UI_TAB_RENDER: {
        tab->off_x = parent->box.left;
        tab->idx = 0;
    } break;
    case UI_TAB_DONE: {
        /* setup body */
        ui_anchor_left_right(&tab->body.box, parent->box.left, parent->box.right);
        ui_anchor_top_bottom(&tab->body.box, tab->at_y, parent->box.bottom);
        if (tab->activate_next)
            ctx->activate_next = ui_true;
        return ui_false;
    } break;}
    return ui_true;
}
static void
ui_tab_control_slot(struct ui_ctx *ctx, struct ui_tab_control *tab,
    const char *txt, const char *end)
{
    struct ui_panel *parent = tab->parent;
    switch (tab->stage) {
    case UI_TAB_DONE:
    default: assert(0); break;
    case UI_TAB_LAYOUT: {
        /* calculate desired size */
        static const int tab_pady = 6;
        struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
        tab->fixed_size = max(ext.h + (tab_pady << 1), tab->fixed_size);
    } break;
    case UI_TAB_INPUT:
    case UI_TAB_RENDER: {
        static const int tab_padx = 8;
        struct ui_panel pan = {0};
        pan.focusable = ui_true;

        struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
        ui_anchor_left_width(&pan.box, tab->off_x, ext.w + (tab_padx << 1));
        ui_anchor_top_height(&pan.box, parent->box.top, tab->fixed_size);
        ui_panel_begin(ctx, &pan, parent);
        tab->off_x += pan.box.width;

        switch (tab->stage) {
        case UI_TAB_DONE:
        case UI_TAB_LAYOUT:
        default: assert(0); break;
        case UI_TAB_INPUT: {
            /* do tab selection */
            if (pan.clicked || pan.pressed) {
                if (tab->selection != tab->idx)
                    tab->toggled = ui_true;
                tab->selection = tab->idx;
            }
            if (pan.state == UI_FOCUSED) {
                struct sys *sys = ctx->sys;
                if (sys->keys[SYS_KEY_LEFT].pressed)
                    tab->selection = max(0, tab->selection-1);
                if (sys->keys[SYS_KEY_RIGHT].pressed)
                    tab->selection = tab->selection+1;
            }
            struct ui_panel dummy = {0};
            ui_panel(ctx, &dummy, &pan);
        } break;
        case UI_TAB_RENDER: {
            /* draw tab button */
            const struct sys *sys = ctx->sys;
            const struct ui_box *p = &pan.box;
            const ui_bool active = (tab->selection == tab->idx);

            const int top = p->top + ((!active) ? 3: 0);
            const int height = p->height - ((!active) ? 3: 0);

            xs_line_style(sys->surf, XLINE_SOLID);
            xs_line_thickness(sys->surf, 1);
            xs_color_background(sys->surf, xc_rgb(0,0,0));

            xs_color(sys->surf, xc_rgb(212,208,200));
            xs_fill_rect(sys->surf, p->left, top, p->width, height, 0);
            xs_color(sys->surf, xc_rgb(255,255,255));
            xs_stroke_line(sys->surf, p->left, top, p->right, top);

            if (!active)
                xs_stroke_line(sys->surf, p->left, p->bottom, p->right, p->bottom);
            if (tab->idx == 0 || (tab->idx-1) != tab->selection)
                xs_stroke_line(sys->surf, p->left, top, p->left, p->bottom);
            if (tab->selection != (tab->idx + 1)) {
                xs_color(sys->surf, xc_rgb(0,0,0));
                xs_stroke_line(sys->surf, p->right-1, top, p->right-1, p->bottom);
                xs_color(sys->surf, xc_rgb(128,128,128));
                xs_stroke_line(sys->surf, p->right-2, top, p->right-2, p->bottom);
            }
            /* draw label */
            struct ui_panel lbl = {0};
            lbl.unselectable = ui_true;
            ui_anchor_center_width(&lbl.box, pan.box.center_x, ext.w);
            ui_anchor_center_height(&lbl.box, pan.box.center_y, ext.h);
            xs_color(sys->surf, xc_rgb(0,0,0));
            ui_label(ctx, &lbl, parent, txt, end);

            /* draw focus */
            if (pan.state == UI_FOCUSED) {
                xs_line_thickness(sys->surf, 1);
                xs_line_style(sys->surf, XLINE_DASHED);
                xs_color(sys->surf, xc_rgb(0,0,0));
                xs_stroke_rect(sys->surf, p->left+3, p->top+5, p->width-7, p->height-9, 0);
            }
        } break;}

        ui_panel_end(&pan);
    } break;}
    tab->idx++;
}
static void
ui_tab_control_end(struct ui_ctx *ctx, struct ui_tab_control *tab)
{
    struct ui_panel *parent = tab->parent;
    switch (tab->stage) {
    case UI_TAB_DONE:
    default: assert(0); break;
    case UI_TAB_LAYOUT: {
        /* layouting pass */
        tab->stage = UI_TAB_INPUT;
        tab->at_y = parent->box.top + tab->fixed_size;
    } break;
    case UI_TAB_INPUT: {
        /* input pass */
        tab->at_x = tab->off_x;
        tab->stage = UI_TAB_RENDER;
        tab->selection = min(tab->selection, max(0, tab->idx-1));
        ui_record_restore(&tab->rec);
        if (ctx->activate_next) {
            tab->activate_next = ui_true;
            ctx->activate_next = ui_false;
            sys_clear(ctx->sys);
        }
    } break;
    case UI_TAB_RENDER: {
        /* render pass */
        tab->stage = UI_TAB_DONE;
        const struct sys *sys = ctx->sys;
        const struct ui_panel *pan = parent;
        const struct ui_box *b = &pan->box;

        /* finally draw tab control border */
        xs_color_background(sys->surf, xc_rgb(0,0,0));
        xs_line_style(sys->surf, XLINE_SOLID);
        xs_line_thickness(sys->surf, 1);

        xs_color(sys->surf, xc_rgb(255,255,255));
        xs_stroke_line(sys->surf, tab->at_x, tab->at_y, b->right-1, tab->at_y);
        xs_stroke_line(sys->surf, b->left, tab->at_y, b->left, b->bottom-1);

        xs_color(sys->surf, xc_rgb(128,128,128));
        xs_stroke_line(sys->surf, b->right-2, tab->at_y+1, b->right-2, b->bottom-2);
        xs_stroke_line(sys->surf, b->left+1, b->bottom-2, b->right-2, b->bottom-2);

        xs_color(sys->surf, xc_rgb(0,0,0));
        xs_stroke_line(sys->surf, b->right-1, tab->at_y, b->right-1, b->bottom-1);
        xs_stroke_line(sys->surf, b->left, b->bottom-1, b->right-1, b->bottom-1);

    } break;}
}
static int
ui_tab_control(struct ui_ctx *ctx, struct ui_tab_control *tab,
    struct ui_panel *parent, const char **titles, int cnt)
{
    while (ui_tab_control_begin(ctx, tab, parent)) {
        for (int i = 0; i < cnt; ++i)
            ui_tab_control_slot(ctx, tab, titles[i], 0);
        ui_tab_control_end(ctx, tab);
    } return tab->selection;
}

/* ---------------------------------------------------------------------------
 *                                  List
 * --------------------------------------------------------------------------- */
static void
ui_list_init(struct ui_list *ls)
{
    ls->at_x = ls->box.left + ls->padx;
    ls->at_y = ls->box.top + ls->pady;
}
static void
ui_list_setup(struct ui_list *ls, const struct ui_panel *pan)
{
    ls->box = pan->box;
    ui_list_init(ls);
}
static void
ui_list_gen(struct ui_box *box, struct ui_list *ls)
{
    ui_anchor_left_width(box, ls->at_x, ls->col_width);
    ui_anchor_top_height(box, ls->at_y, ls->row_height);

    switch (ls->orient) {
    case UI_HORIZONTAL: {
        if ((ls->flow == UI_FLOW_WRAP) &&
            (box->right + ls->col_width + ls->spacing_x + ls->padx) >= ls->box.right) {
            ls->at_x = ls->box.left + ls->padx;
            ls->at_y += ls->row_height + ls->spacing_y;
        } else ls->at_x += ls->col_width + ls->spacing_x;
    } break;
    case UI_VERTICAL: {
        if ((ls->flow == UI_FLOW_WRAP) &&
            (box->bottom + ls->row_height + ls->spacing_y + ls->pady) >= ls->box.bottom) {
            ls->at_y = ls->box.top + ls->pady;
            ls->at_x += ls->col_width + ls->spacing_x;
        } else ls->at_y += ls->row_height + ls->spacing_y;
    } break;}
}

/* ---------------------------------------------------------------------------
 *                                  View
 * --------------------------------------------------------------------------- */
static void
ui_view(struct ui_view *v, struct ui_list* ls, int width, int height)
{
    int total = max(1, v->total)-1;
    v->space_x = max(0, width - (ls->padx << 1));
    v->space_y = max(0, height - (ls->pady << 1));
    v->col_width = max(1, ls->col_width + ls->spacing_x);
    v->col_cnt = max(1, floori(cast(float, v->space_x) / cast(float, v->col_width)));
    v->row_cnt = max(1, ceili(cast(float, total) / cast(float, max(1, v->col_cnt))));
    v->row_height = max(1, ls->spacing_y + ls->row_height);
    v->total_height = max(1, v->row_cnt) * v->row_height;
    v->total_width = max(1, v->col_cnt) * v->col_width;
    v->orient = ls->orient;
    v->flow = ls->flow;

    int row_off = floori(v->offset / cast(float, v->row_height));
    int row_win = ceili(cast(float, v->space_y) / cast(float, v->row_height));

    v->begin = row_off * v->col_cnt;
    v->end = v->begin + v->col_cnt * (row_win + 1);
    v->end = min(v->end, v->total);

    v->max_x = ls->at_x + v->total_width;
    v->max_y = ls->at_y + v->total_height;
    ls->at_y += row_off * v->row_height;
}
static void
ui_view_panel(struct ui_view *v, struct ui_list* ls, struct ui_panel* pan)
{
    ui_view(v, ls, pan->box.width, pan->box.height);
}
static float
ui_view_center(struct ui_view *v, int index)
{
    const int total = max(1, v->total)-1;
    const int idx = clamp(0, index, total);

    switch (v->orient) {
    default: assert(0); return 0.0f;
    case UI_HORIZONTAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int row = idx / v->row_cnt;
            int center_off = v->row_height * row + (v->row_height >> 1);
            int off = center_off - (v->space_y >> 1);
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        }
        case UI_FLOW_STRAIGHT: {
            int center_off = v->col_width * idx + (v->col_width >> 1);
            int off = center_off - (v->space_x >> 1);
            return cast(float, clamp(0, off, v->total_width - v->space_x));
        } default: return 0.0f;}

    case UI_VERTICAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int col = idx / v->col_cnt;
            int center_off = v->col_width * col + (v->col_width >> 1);
            int off = center_off - (v->space_x >> 1);
            return cast(float, clamp(0, off, v->total_width - v->space_x));
        }
        case UI_FLOW_STRAIGHT: {
            int center_off = v->row_height * idx + (v->row_height >> 1);
            int off = center_off - (v->space_y >> 1);
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        } default: return 0.0f;}
    }
}
static float
ui_view_fit_start(struct ui_view *v, int index)
{
    int total = max(1, v->total)-1;
    int idx = clamp(0, index, total);

    switch (v->orient) {
    default: assert(0); return 0.0f;
    case UI_HORIZONTAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int row = idx / v->row_cnt;
            int off = v->row_height * row;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        }
        case UI_FLOW_STRAIGHT: {
            int off = v->col_width * idx;
            return cast(float, clamp(0, off, v->total_width - v->space_x));
        } default: return 0.0f;}

    case UI_VERTICAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int col = idx / v->col_cnt;
            int off = v->col_width * col;
            return cast(float, clamp(0, off, v->total_width - v->space_x));
        }
        case UI_FLOW_STRAIGHT: {
            int off = v->row_height * idx;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        } default: return 0.0f;}
    }
}
static float
ui_view_fit_end(struct ui_view *v, int index)
{
    int total = max(1, v->total)-1;
    int idx = clamp(0, index, total);

    switch (v->orient) {
    default: assert(0); return 0.0f;
    case UI_HORIZONTAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int row = idx / v->row_cnt;
            int off = v->row_height * (row+1) - v->space_y;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        }
        case UI_FLOW_STRAIGHT: {
            int off = v->col_width * (idx+1) - v->space_x;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        } default: return 0.0f;}

    case UI_VERTICAL:
        switch (v->flow) {
        case UI_FLOW_WRAP: {
            int col = idx / v->col_cnt;
            int off = v->col_width * (col+1) - v->space_x;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        }
        case UI_FLOW_STRAIGHT: {
            int off = v->row_height * (idx+1) - v->space_y;
            return cast(float, clamp(0, off, v->total_height - v->space_y));
        } default: return 0.0f;}
    }
}
static float
ui_view_goto_begin(struct ui_view *v)
{
    return ui_view_fit_start(v, 0);
}
static float
ui_view_goto_end(struct ui_view *v)
{
    return ui_view_fit_end(v, v->total);
}

/* ---------------------------------------------------------------------------
 *                                  Tree
 * --------------------------------------------------------------------------- */
#define UI_TREE_PLUS_MINUS_SIZE 9

static void
ui_tree_show_lines(struct ui_tree *t, int *depth_buf, int max_depth)
{
    t->show_lines = ui_true;
    t->max_depth = max_depth;
    t->y_at_depth = depth_buf;
    t->prev_depth = -1;
}
static int
ui_tree_begin(struct ui_ctx *ctx,
    struct ui_tree *t, struct ui_panel *parent)
{
    static const int pady = 4;
    t->area.off_x = t->off_x;
    t->area.off_y = t->off_y;
    t->repaint = ui_false;
    t->idx = 0;

    const int ret = ui_area_begin(ctx, &t->area, &t->area_panel, parent);
    t->at_y = t->area_panel.box.top + pady;
    return ret;
}
static int
ui_tree_node_plus_minus_button(struct ui_ctx *ctx, struct ui_panel *pan,
    struct ui_panel *parent, enum ui_tree_node_state state)
{
    ui_panel_begin(ctx, pan, parent); {
        const struct sys *sys = ctx->sys;
        if (pan->clicked)
            state = !state;

        /* draw correct icon */
        const struct xsurface *icon = 0;
        switch (state) {
        default: assert(0); return 0;
        case UI_TREE_NODE_COLLAPSED:
            icon = res_img(ctx->res, IMG_PLUS); break;
        case UI_TREE_NODE_EXPANDED:
            icon = res_img(ctx->res, IMG_MINUS); break;}
        xs_blit(sys->surf, icon, pan->box.left, pan->box.top, 0,0, icon->w, icon->h);
    } ui_panel_end(pan);
    return pan->clicked;
}
static void
ui_tree_node(struct ui_ctx *ctx, struct ui_tree *t,
    struct ui_tree_node *node, struct ui_panel *pan,
    const char *txt, const char *end, int depth)
{
    static const int padx = 6;
    static const int pady = 4;
    static const int spacing_x = 6;
    static const int spacing_y = 0;
    static const int depth_scaler = 18;
    struct sys *sys = ctx->sys;

    /* reset node state */
    node->check_changed = 0;
    node->state_changed = 0;
    node->selection_changed = 0;

    /* optional flags */
    const ui_bool has_chk = t->flags & UI_TREE_NODE_CHECK;
    const ui_bool has_icon = t->flags & UI_TREE_NODE_ICON;
    const ui_bool has_state = (node->type == UI_TREE_NODE_INTERNAL);

    /* calculate node extend */
    const struct xsurface *xs = res_icon(ctx->res);
    struct extend ext = xf_text_measure(res_font(ctx->res), txt, end);
    int node_height = ext.h;
    node_height = max(node_height, UI_TREE_PLUS_MINUS_SIZE);
    node_height = max(node_height, UI_CHECKBOX_SIZE);
    node_height = max(node_height, xs->h);

    int icon_width = 0;
    icon_width = max(icon_width, UI_TREE_PLUS_MINUS_SIZE);
    icon_width = max(icon_width, UI_CHECKBOX_SIZE);
    icon_width = max(icon_width, xs->w);

    int hline_begin = 0;
    int hline_end = 0;
    int vline_x = 0;
    int vline_y = 0;
    int y_depth = 0;

    memset(pan, 0, sizeof(*pan));
    if (node->selected)
        pan->focusable = ui_true;
    else pan->focusable = ui_false;
    ui_panel_begin(ctx, pan, &t->area_panel);

    if (t->focus_next) {
        ctx->active = pan->id;
        pan->state = UI_FOCUSED;
        t->focus_next = ui_false;
    }
    if (t->inconsistent) {
        /* handle selection changes */
        if (ui_id_eq(&pan->id, &t->selected_node)) {
            node->selected = ui_true;
            node->selection_changed = ui_true;
            t->selection_changed = ui_true;
            t->inconsistent = ui_false;
        } else if (node->selected) {
            node->selected = ui_false;
            node->selection_changed = ui_true;
        } else node->selected = ui_false;
    }
    const int center_y = t->at_y + (node_height >> 1);
    int at_x = t->area_panel.box.left + padx + depth * depth_scaler;
    if (has_state) {
        /* Plus-minus icon (optional) */
        struct ui_panel ico = {0};
        ui_anchor_left_width(&ico.box, at_x, UI_TREE_PLUS_MINUS_SIZE);
        ui_anchor_center_height(&ico.box, center_y, UI_TREE_PLUS_MINUS_SIZE);
        if (ui_tree_node_plus_minus_button(ctx, &ico, &t->area_panel, node->state)) {
            node->selected = !node->selected;
            node->selection_changed = ui_true;
            node->state_changed = ui_true;
            node->state = !node->state;
        }
        if (pan->state == UI_FOCUSED) {
            if (sys->keys[SYS_KEY_RIGHT].pressed && !node->state) {
                node->state_changed = ui_true;
                node->state = ui_true;
            }
            if (sys->keys[SYS_KEY_LEFT].pressed && node->state) {
                node->state_changed = ui_true;
                node->state = ui_false;
            }
        }
        vline_y = ico.box.top;
        y_depth = ico.box.bottom;
        hline_begin = ico.box.right;
        vline_x = at_x + (UI_TREE_PLUS_MINUS_SIZE >> 1);
        at_x = ico.box.right + spacing_x;
    } else {
        y_depth = center_y;
        vline_y = center_y;
        hline_begin = at_x + (UI_TREE_PLUS_MINUS_SIZE >> 1);
        vline_x = hline_begin;
        at_x += UI_TREE_PLUS_MINUS_SIZE + spacing_x;
    }
    if (has_chk) {
        /* Checkbox (optional) */
        struct ui_panel chk = {0};
        ui_anchor_left_width(&chk.box, at_x, UI_CHECKBOX_SIZE);
        ui_anchor_center_height(&chk.box, center_y, UI_CHECKBOX_SIZE);
        if (ui_check(ctx, &chk, &t->area_panel, node->checked) != node->checked) {
            node->checked = !node->checked;
            node->check_changed = ui_true;
        }
        hline_end = chk.box.left;
        at_x = chk.box.right + spacing_x;
    }
    if (has_icon) {
        /* Icon (optional) */
        struct ui_panel ico = {0};
        ui_anchor_left_width(&ico.box, at_x, xs->w);
        ui_anchor_center_height(&ico.box, center_y, xs->h);
        ui_icon(ctx, &ico, &t->area_panel);
        if (ico.clicked) {
            node->selected = !node->selected;
            node->selection_changed = ui_true;
        }
        if (!has_chk) hline_end = ico.box.left;
        at_x = ico.box.right + spacing_x;
    }
    if (t->show_lines) {
        /* horizontal line */
        if (!has_chk && !has_icon)
            hline_end = at_x;
        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_DASHED);
        xs_color(sys->surf, xc_rgb(123,123,123));
        xs_stroke_line(sys->surf, hline_begin, center_y, hline_end, center_y);

        /* vertical line */
        if (depth == t->prev_depth) {
            int off = 0;
            if (t->prev_state)
                off = (node_height - UI_TREE_PLUS_MINUS_SIZE) >> 1;
            else off = (node_height >> 1);
            int vline_end = t->at_y - spacing_y - off;
            xs_stroke_line(sys->surf, vline_x, vline_y, vline_x, vline_end);
        } else if (depth > t->prev_depth && depth) {
            int off = (node_height - xs->h) >> 1;
            int vline_end = t->at_y - spacing_y - off;
            xs_stroke_line(sys->surf, vline_x, vline_y, vline_x, vline_end);
        } else if (depth < t->prev_depth){
            int vline_end = t->y_at_depth[depth];
            xs_stroke_line(sys->surf, vline_x, vline_y, vline_x, vline_end);
        }
        t->y_at_depth[depth] = y_depth;
        t->prev_state = has_state;
        t->prev_depth = depth;
    }
    /* Label (required) */
    struct ui_panel lbl = {0};
    ui_anchor_left_width(&lbl.box, at_x, ext.w);
    ui_anchor_center_height(&lbl.box, center_y, ext.h);
    if (node->selected) {
        /* selected item label */
        const unsigned fg = xs_color_foreground(sys->surf, xc_rgb(0,0,0));
        const unsigned bg = xs_color_background(sys->surf, xc_rgb(255,255,255));

        /* text hightlight */
        xs_color(sys->surf, xc_rgb(0,0,128));
        xs_fill_rect(sys->surf, lbl.box.left-1, lbl.box.top,
            lbl.box.width+2, lbl.box.height, 0);
        xs_color(sys->surf, xc_rgb(255,255,255));
        ui_label(ctx, &lbl, &t->area_panel, txt, 0);

        xs_color_foreground(sys->surf, fg);
        xs_color_background(sys->surf, bg);
    } else {
        xs_color_foreground(sys->surf, xc_rgb(0,0,0));
        ui_label(ctx, &lbl, &t->area_panel, txt, 0);
    }
    if (lbl.clicked) {
        /* node selection */
        ctx->active = pan->id;
        node->selected = !node->selected;
        node->selection_changed = ui_true;
    }
    if (pan->state == UI_FOCUSED) {
        /* cursor movement */
        if (sys->keys[SYS_KEY_ACTIVATE].pressed) {
            node->selection_changed = ui_true;
            node->selected = !node->selected;
            sys_clear(sys);
        }
        if (sys->keys[SYS_KEY_UP].pressed) {
            pan->state = UI_NORMAL;
            ctx->active = t->prev;
            t->repaint = ui_true;
            sys_clear(sys);
        }
        if (sys->keys[SYS_KEY_DOWN].pressed) {
            pan->state = UI_NORMAL;
            t->focus_next = ui_true;
            t->repaint = ui_true;
            sys_clear(sys);
        }
        /* cursor scrolling */
        if (t->at_y + node_height > t->area_panel.box.bottom - pady + floori(t->area.off_y))
            t->off_y = cast(float, (t->at_y - (t->area_panel.box.top + pady)) - t->area_panel.box.height + node_height);
        else if (t->at_y < t->area_panel.box.top - pady + floori(t->area.off_y))
            t->off_y = cast(float, t->at_y - (t->area_panel.box.top + pady));

        /* draw cursor */
        struct ui_box *b = &lbl.box;
        xs_line_thickness(sys->surf, 1);
        xs_line_style(sys->surf, XLINE_DASHED);
        xs_color(sys->surf, xc_rgb(0,0,0));
        xs_stroke_rect(sys->surf, b->left-1, b->top-1, b->width+1, b->height+1, 0);
    }
    if (node->selected && node->selection_changed &&
        !ui_id_eq(&pan->id, &t->selected_node)) {
        /* Selection invalidated current tree state, so signal another iteration */
        t->off_y = cast(float, center_y - (t->area_panel.box.top + pady) - (t->area_panel.box.height >> 1));
        t->selected_node = pan->id;
        t->inconsistent = ui_true;

        node->selection_changed = ui_false;
        node->selected = ui_false;
        sys_clear(sys);
    }
    ui_panel_end(pan);

    t->at_y += max(ext.h, node_height) + spacing_y;
    t->prev = pan->id;
    t->idx++;
}
static void
ui_tree_end(struct ui_ctx *ctx, struct ui_tree *t)
{
    if (t->inconsistent || t->repaint)
        t->area.state = UI_INCONSISTENT;

    ui_area_end(ctx, &t->area);
    if (t->area.scrolled) {
        t->off_x = t->area.off_x;
        t->off_y = t->area.off_y;
    } else t->off_y = clamp(0, t->off_y, t->area.max_offy);
}
