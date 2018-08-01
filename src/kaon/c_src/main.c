/* sys */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>

/* os */
#include <unistd.h>
#include <sys/time.h>

/* x11 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#ifdef offsetof
  #undef offsetof
#endif
#ifdef min
  #undef min
  #undef max
#endif

/* usr */
#include "util.c"
#include "drw.h"
#include "drw.c"
#include "sys.h"
#include "sys.c"
#include "props.h"
#include "props.c"
#include "res.h"
#include "res.c"
#include "ui.h"
#include "ui.c"

/* ============================================================================
 *
 *                                  APP
 *
 * =========================================================================== */
struct file_def {
    int type;
    const char *suffix;
    const char *name;
    int icon;
};
enum file_info_state {
    FILE_INFO_SETUP,
    FILE_INFO_LOADED
};
struct file_info {
    int type;
    enum file_info_state state;
    struct directory *owner;
    char *path;
    char *name;
    char *fullpath;
    char *extension;
    struct stat stat;
    unsigned int is_dir:1;
};
struct directory {
    struct directory *parent;
    char *path;
    char *name;
    char *fullpath;
    int depth, cnt;
    struct directory **dirs;
    struct ui_tree_node ui;
};
enum file_type {FILE_DEFAULT, FILE_FOLDER, FILE_TEXT, FILE_C_SOURCE,
    FILE_CPP_SOURCE, FILE_HEADER, FILE_CPP_HEADER, FILE_PYTHON, FILE_RUBY,
    FILE_JAVA, FILE_LUA, FILE_JS, FILE_HTML, FILE_CSS, FILE_MD, FILE_DOC, FILE_DOCX,
    FILE_MP3, FILE_WAV, FILE_OGG, FILE_TTF, FILE_BMP, FILE_PNG, FILE_JPEG,
    FILE_PCX, FILE_TGA, FILE_GIF, FILE_CHM, FILE_ZIP, FILE_RAR, FILE_GZ,
    FILE_TAR, FILE_PDF, FILE_CNT
};
static const struct file_def app_file_defs[] = {
    [FILE_DEFAULT]      = {FILE_DEFAULT,    "",         "Unknown",      ICON_FILE_DEF},
    [FILE_FOLDER]       = {FILE_FOLDER,     NULL,       "Folder",       ICON_DIR},
    [FILE_TEXT]         = {FILE_TEXT,       "txt",      "Text",         ICON_FILE_TXT},
    [FILE_C_SOURCE]     = {FILE_C_SOURCE,   "c",        "C Source",     ICON_FILE_TXT},
    [FILE_CPP_SOURCE]   = {FILE_CPP_SOURCE, "cpp",      "C++ Source",   ICON_FILE_TXT},
    [FILE_HEADER]       = {FILE_HEADER,     "h",        "C Header",     ICON_FILE_TXT},
    [FILE_CPP_HEADER]   = {FILE_CPP_HEADER, "hpp",      "C++ Header",   ICON_FILE_TXT},
    [FILE_PYTHON]       = {FILE_PYTHON,     "py",       "Python Script",ICON_FILE_TXT},
    [FILE_RUBY]         = {FILE_RUBY,       "rby",      "Ruby Script",  ICON_FILE_TXT},
    [FILE_JAVA]         = {FILE_JAVA,       "java",     "Java Source",  ICON_FILE_TXT},
    [FILE_LUA]          = {FILE_LUA,        "lua",      "Lua Script",   ICON_FILE_TXT},
    [FILE_JS]           = {FILE_JS,         "js",       "JavaScript",   ICON_FILE_TXT},
    [FILE_HTML]         = {FILE_HTML,       "html",     "HTML",         ICON_FILE_TXT},
    [FILE_CSS]          = {FILE_CSS,        "css",      "CSS",          ICON_FILE_TXT},
    [FILE_MD]           = {FILE_MD,         "md",       "Markdown",     ICON_FILE_TXT},
    [FILE_DOC]          = {FILE_DOC,        "doc",      "Document",     ICON_FILE_TXT},
    [FILE_DOCX]         = {FILE_DOCX,       "docx",     "DocumentX",    ICON_FILE_TXT},
    [FILE_MP3]          = {FILE_MP3,        "mp3",      "MP3",          ICON_FILE_SFX},
    [FILE_WAV]          = {FILE_WAV,        "wav",      "WAV",          ICON_FILE_SFX},
    [FILE_OGG]          = {FILE_OGG,        "ogg",      "OGG",          ICON_FILE_SFX},
    [FILE_TTF]          = {FILE_TTF,        "ttf",      "TTF",          ICON_FILE_FNT},
    [FILE_BMP]          = {FILE_BMP,        "bmp",      "BMP",          ICON_FILE_IMG},
    [FILE_PNG]          = {FILE_PNG,        "png",      "PNG",          ICON_FILE_PNG},
    [FILE_JPEG]         = {FILE_JPEG,       "jpg",      "JPEG",         ICON_FILE_JPG},
    [FILE_PCX]          = {FILE_PCX,        "pcx",      "PCX",          ICON_FILE_PCX},
    [FILE_TGA]          = {FILE_TGA,        "tga",      "TGA",          ICON_FILE_IMG},
    [FILE_GIF]          = {FILE_GIF,        "gif",      "GIF",          ICON_FILE_GIF},
    [FILE_CHM]          = {FILE_CHM,        "chm",      "Help",         ICON_FILE_CHM},
    [FILE_ZIP]          = {FILE_ZIP,        "zip",      "Archive",      ICON_FILE_ZIP},
    [FILE_RAR]          = {FILE_RAR,        "rar",      "Archive",      ICON_FILE_ZIP},
    [FILE_GZ]           = {FILE_GZ,         "gz",       "Archive",      ICON_FILE_ZIP},
    [FILE_TAR]          = {FILE_TAR,        "tar",      "Archive",      ICON_FILE_ZIP},
    [FILE_PDF]          = {FILE_PDF,        "pdf",      "PDF",          ICON_FILE_PDF},
};
/* tree */
static struct arena app_tree_arena;
static struct arena app_tmp_arena;
static struct directory *app_tree_root;
static struct directory *app_dir_cur = 0;
static int app_tree_depth;
static int app_tree_cnt;

/* icon size */
enum icon_size {ICON_SIZE_SMALL, ICON_SIZE_MEDIUM, ICON_SIZE_LARGE};
static const char *app_icon_size_titles[] = {"Small", "Medium", "Large"};
static int app_icon_size = ICON_SIZE_MEDIUM;

/* layout */
enum dir_layouting {DIR_LAYOUT_VERTICAL, DIR_LAYOUT_HORIZONTAL, DIR_LAYOUT_SWITCH};
static const char *app_dir_layout_titles[] = {"Vertical", "Horizontal", "Switch"};
static int app_dir_layout = DIR_LAYOUT_SWITCH;

/* directory */
enum directory_view {DIR_VIEW_ICON, DIR_VIEW_REPORT};
static const char *app_dir_view_titles[] = {"Icon", "Details"};
static int app_directory_view = DIR_VIEW_ICON;

static struct arena app_dir_arena;
static char app_cur_dir_path[MAX_PATH];
static struct file_info *app_cur_dir_content = 0;
static float app_cur_dir_off_y = 0;

/* undo/redo */
#define APP_MAX_UNDO_STACK (8*1024)
static struct directory *app_directory_undo[APP_MAX_UNDO_STACK];
static int app_undo_head = 0;
static int app_undo_at = 0;

/* ---------------------------------------------------------------------------
 *                                  Vars
 * --------------------------------------------------------------------------- */
static const struct var g_var_icon_size = {
    .type   = VAR_ENUM,
    .flags  = VAR_ARCHIVE,
    .name   = "icon_size",
    .print  = "Icon size:",
    .help   = "Directory view file icon size",
    .val = {.e = {
        .options = app_icon_size_titles,
        .num = cntof(app_icon_size_titles),
        .sel = &app_icon_size
    }}
};
static const struct var g_var_dir_view = {
    .type   = VAR_ENUM,
    .flags  = VAR_ARCHIVE,
    .name   = "dir_view",
    .print  = "Directory View:",
    .help   = "Directory view list mode",
    .val = {.e = {
        .options = app_dir_view_titles,
        .num = cntof(app_dir_view_titles),
        .sel = &app_directory_view
    }}
};
static const struct var g_var_dir_layout = {
    .type   = VAR_ENUM,
    .flags  = VAR_ARCHIVE,
    .name   = "dir_layout",
    .print  = "Filesystem Layout:",
    .help   = "Filesystem layout",
    .val = {.e = {
        .options = app_dir_layout_titles,
        .num = cntof(app_dir_layout_titles),
        .sel = &app_dir_layout
    }}
};

/* ---------------------------------------------------------------------------
 *                                  Files
 * --------------------------------------------------------------------------- */
static int
file_type(const char *ext)
{
    for (int i = 0; i < FILE_CNT; ++i) {
        const struct file_def *def = app_file_defs + i;
        if (def->suffix && strcmp(def->suffix, ext) == 0)
            return i;
    } return FILE_DEFAULT;
}
static int
file_icon(int type)
{
    assert(type >= 0 && type < FILE_CNT);
    return app_file_defs[type].icon;
}

/* ---------------------------------------------------------------------------
 *                                  Directory
 * --------------------------------------------------------------------------- */
static int
dir_compare_fileinfo_asc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    if (fa->is_dir && !fb->is_dir) return -1;
    else if (!fa->is_dir && fb->is_dir) return 1;
    return strcmp(fa->name, fb->name);
}
static int
dir_compare_fileinfo_desc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    if (fa->is_dir && !fb->is_dir) return -1;
    else if (!fa->is_dir && fb->is_dir) return 1;
    return strcmp(fb->name, fa->name);
}
static int
dir_compare_fileinfo_size_asc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    return cast(int, fa->stat.st_size - fb->stat.st_size);
}
static int
dir_compare_fileinfo_size_desc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    return cast(int, fb->stat.st_size - fa->stat.st_size);
}
static int
dir_compare_fileinfo_time_asc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    return cast(int, fa->stat.st_mtim.tv_sec - fb->stat.st_mtim.tv_sec);
}
static int
dir_compare_fileinfo_time_desc(const void *a, const void *b)
{
    const struct file_info *fa = (const struct file_info*)a;
    const struct file_info *fb = (const struct file_info*)b;
    return cast(int, fb->stat.st_mtim.tv_sec - fa->stat.st_mtim.tv_sec);
}
static void
dir_stat_file(struct file_info *f)
{
    int res = stat(f->fullpath, &f->stat);
    if (res < 0) return;
    f->state = FILE_INFO_LOADED;
}
static void
dir_load_files(struct directory *dir, struct arena *a)
{
    arena_free(a);
    buf_free(app_cur_dir_content);
    strscpy(app_cur_dir_path, dir->fullpath, MAX_PATH);

    struct dir_iter it = {0};
    for (dir_begin(&it, dir->fullpath); it.valid; dir_next(&it)) {
        struct file_info info = {0};
        if (it.name[0] == '.')
            continue; /* skip hidden files */

        /* create file info */
        info.state = FILE_INFO_SETUP;
        info.owner = dir;
        info.is_dir = it.is_dir;
        info.path = arena_printf(a, "%s/", it.base);
        info.name = arena_push_str(a, it.name, 0);
        info.fullpath = arena_printf(a, "%s%s", info.path, info.name);
        dir_stat_file(&info);
        if (!info.is_dir) {
            const char *ext = path_ext(info.name);
            info.extension = arena_push_str(a, ext, 0);
            info.type = file_type(info.extension);
        } else {
            info.extension = 0;
            info.type = FILE_FOLDER;
        } buf_push(app_cur_dir_content, info);

    } qsort(app_cur_dir_content, (size_t)buf_cnt(app_cur_dir_content),
        sizeof(struct file_info), dir_compare_fileinfo_asc);
    app_cur_dir_off_y = 0;
}
static void
dir_activate(struct directory* d)
{
    dir_load_files(d, &app_dir_arena);
    app_dir_cur = d;
    app_dir_cur->ui.state = UI_TREE_NODE_EXPANDED;
}

/* ---------------------------------------------------------------------------
 *                                  Filesytem
 * --------------------------------------------------------------------------- */
static void
fs_load(struct directory *d, struct arena *a)
{
    struct dir_iter it = {0};
    for (dir_begin(&it, d->fullpath); it.valid; dir_next(&it)) {
        if (it.name[0] == '.')
            continue; /* skip hidden files */
        else d->cnt++;

        if (!it.is_dir) continue;
        struct directory *s = arena_push(a, szof(struct directory));
        s->ui.state = UI_TREE_NODE_COLLAPSED;
        s->path = arena_printf(a, "%s/", it.base);
        s->name = arena_push_str(a, it.name, 0);
        s->fullpath = arena_printf(a, "%s%s", s->path, s->name);
        s->depth = d->depth + 1;
        s->parent = d;

        buf_push(d->dirs, s);
        app_tree_depth = max(app_tree_depth, s->depth);
        app_tree_cnt++;
    }
}
static void
fs_load_fullpath_r(struct directory *d,
    struct path_iter *it, struct arena *a)
{
    fs_load(d, a);
    if (!path_next(it)) {
        app_dir_cur = d;
        d->ui.selected = ui_true;
        d->ui.state = UI_TREE_NODE_COLLAPSED;
        return;
    } else d->ui.state = UI_TREE_NODE_EXPANDED;

    for (int i = 0; i < buf_cnt(d->dirs); ++i) {
        char subdir[MAX_PATH];
        memcpy(subdir, it->begin, cast(size_t, it->end - it->begin));
        subdir[it->end - it->begin] = 0;
        if (strcmp(subdir, d->dirs[i]->name) == 0)
            fs_load_fullpath_r(d->dirs[i], it, a);
        else fs_load(d->dirs[i], a);
    }
}
static void
fs_load_fullpath(const char *path, struct arena *a)
{
    struct path_iter it = {0};
    struct directory *root = arena_push(a, szof(struct directory));
    path_begin(&it, path, 0);
    if (path_next(&it)) {
        root->ui.state = UI_TREE_NODE_COLLAPSED;
        root->path = arena_push_str(a, path, it.end);
        root->name = arena_push_str(a, it.begin, it.end);
        root->fullpath = root->path;
        fs_load_fullpath_r(root, &it, a);
    } app_tree_root = root;
}
static void
fs_load_dir_at_path(const char *dir_fullpath, struct arena *a)
{
    int i = 0;
    struct path_iter it = {0};
    struct directory *d = app_tree_root;
    path_begin(&it, dir_fullpath, 0);

    path_next(&it);
    while (path_next(&it)) {
        char name[MAX_PATH];
        memcpy(name, it.begin, cast(size_t, it.end-it.begin));
        name[it.end-it.begin] = 0;
        for (i = 0; i < buf_cnt(d->dirs); ++i) {
            if (strcmp(name, d->dirs[i]->name) == 0)
                {d = d->dirs[i]; break;}
        }
        /* directory not in tree so create it */
        if (i == buf_cnt(d->dirs)) {
            fs_load_fullpath_r(d, &it, a);
            return;
        }
    }
    if (!d->cnt) fs_load(d, a);
    for (i = 0; i < buf_cnt(d->dirs); ++i) {
        if (!d->dirs[i]->cnt)
            fs_load(d->dirs[i], a);
    }
}

/* ---------------------------------------------------------------------------
 *                                  UNDO
 * --------------------------------------------------------------------------- */
static void
undo_clear(void)
{
    app_undo_head = 0;
    app_undo_at = 0;
}
static void
undo_push(struct directory *dir)
{
    if ((app_undo_at + 1) < app_undo_head)
        app_undo_head = app_undo_at;
    app_undo_head = min(app_undo_head, APP_MAX_UNDO_STACK-1);
    app_directory_undo[app_undo_head] = dir;
    app_undo_at = app_undo_head++;
}
static void
undo(void)
{
    app_undo_at = max(0, app_undo_at - 1);
    struct directory *d = app_directory_undo[app_undo_at];
    dir_activate(d);
}
static void
redo(void)
{
    if (app_undo_at + 1 >= app_undo_head) return;
    struct directory *d = app_directory_undo[++app_undo_at];
    dir_activate(d);
}

/* ---------------------------------------------------------------------------
 *                                  UI: Menu
 * --------------------------------------------------------------------------- */
static void
ui_menu(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_layout(ctx, UI_LAYOUT_FIT);
    ui_panel_begin(ctx, pan, root); {
        static const int spacing = 4;

        /* Home */
        struct ui_panel home = {0};
        ui_anchor_left_width(&home.box, pan->box.left, pan->box.height);
        ui_anchor_top_height(&home.box, pan->box.top, pan->box.height);
        res_activate_icon(ctx->res, ICON_COMPUTER);
        ui_button_icon(ctx, &home, root);
        if (home.clicked) {
            dir_activate(app_tree_root);
            undo_clear();
        }
        /* Search */
        ui_disable(ctx);
        struct ui_panel fnd = {0};
        ui_anchor_left_width(&fnd.box, home.box.right + spacing, 80);
        ui_anchor_top_height(&fnd.box, home.box.top, pan->box.height);
        res_activate_icon(ctx->res, ICON_SEARCH);
        ui_button_icon_label(ctx, &fnd, root, "Search", 0);
        if (fnd.clicked)
            printf("Search!\n");
        ui_enable(ctx);

        /* Checkbox */
        static int checked = 1;
        struct ui_panel chk = {0};
        ui_anchor_left_width(&chk.box, fnd.box.right + spacing, 70);
        ui_anchor_top_height(&chk.box, fnd.box.top, pan->box.height);
        ui_checkbox(ctx, &chk, root, &checked, "Checkbox", 0);

        /* Edit Box */
        static int len = 5;
        static char buf[64] = "Input";
        struct ui_edit_box edit_box = {0};
        struct ui_panel edit = {0};
        ui_anchor_left_width(&edit.box, chk.box.right + spacing, 100);
        ui_anchor_top_height(&edit.box, chk.box.top, pan->box.height);
        ui_edit_box(ctx, &edit_box, &edit, root, buf, &len, szof(buf));

        /* Spinner */
        static int n = 10;
        struct ui_panel spin = {0};
        struct ui_spinner spinner = {0};
        ui_anchor_left_width(&spin.box, edit.box.right + spacing, 60);
        ui_anchor_top_height(&spin.box, edit.box.top, pan->box.height);
        n = ui_spinner(ctx, &spinner, &spin, root, n);

        /* combo */
        static int weekday = 0;
        static const char *items[] = {"Monday","Tuesday","Wednessday", "Thursday", "Friday", "Saturday", "Sunday"};
        struct ui_panel c = {0};
        struct ui_combo com = {.selected = weekday};
        ui_anchor_left_width(&c.box, spin.box.right + spacing, 100);
        ui_anchor_top_height(&c.box, spin.box.top, pan->box.height);
        ui_combo(ctx, &com, &c, pan, items, cntof(items));
        weekday = com.selected;

    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  UI: Toolbar
 * --------------------------------------------------------------------------- */
static void
ui_tool(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_layout(ctx, UI_LAYOUT_STRETCH);
    ui_panel_begin(ctx, pan, root); {
        static const int spacing = 2;
        static const int btn_size = 60;

        /* Back */
        struct ui_panel prv = {0};
        ui_anchor_left_width(&prv.box, pan->box.left, btn_size);
        ui_anchor_top_height(&prv.box, pan->box.top, pan->box.height);
        res_activate_img(ctx->res, IMG_BACK);
        ui_tool_button(ctx, &prv, root, "Back", 0);
        if (prv.clicked) undo();

        /* Forward */
        struct ui_panel nxt = {0};
        ui_anchor_left_width(&nxt.box, prv.box.right + spacing, btn_size);
        ui_anchor_top_height(&nxt.box, prv.box.top, pan->box.height);
        res_activate_img(ctx->res, IMG_FORWARD);
        ui_tool_button(ctx, &nxt, root, "Forward", 0);
        if (nxt.clicked) redo();

        /* Up */
        struct ui_panel up = {0};
        ui_anchor_left_width(&up.box, nxt.box.right + spacing, btn_size);
        ui_anchor_top_height(&up.box, nxt.box.top, pan->box.height);
        res_activate_img(ctx->res, IMG_UP);
        ui_tool_button(ctx, &up, root, "Up", 0);
        if (up.clicked && app_dir_cur->parent) {
            /* change directory to parent */
            struct directory *p = app_dir_cur->parent;
            dir_activate(p);
            undo_push(p);
        }
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                              UI: FileSystem
 * --------------------------------------------------------------------------- */
static void
ui_fs_node_r(struct ui_ctx *ctx, struct ui_tree *tree,
    int depth, struct directory *d)
{
    /* execute tree node */
    struct ui_panel pan = {0};
    struct ui_tree_node *node = &d->ui;
    node->selected = (app_dir_cur == d);
    if (buf_cnt(d->dirs))
        node->type = UI_TREE_NODE_INTERNAL;
    else node->type = UI_TREE_NODE_LEAF;
    ui_tree_node(ctx, tree, node, &pan, d->name, 0, depth);

    /* handle node state changes */
    if (node->state_changed && (node->state == UI_TREE_NODE_EXPANDED))
        fs_load_dir_at_path(d->fullpath, &app_tree_arena);
    if (node->selection_changed && node->selected) {
        fs_load_dir_at_path(d->fullpath, &app_tree_arena);
        dir_activate(d);
        undo_push(d);
    }
    /* recurse down into child nodes */
    if (node->state == UI_TREE_NODE_EXPANDED) {
        for (int i = 0; i < buf_cnt(d->dirs); ++i)
            ui_fs_node_r(ctx, tree, depth+1, d->dirs[i]);
    }
}
static int
ui_fs(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    struct ui_tree tree = {0};
    ui_panel_begin(ctx, pan, root); {
        static float s_off_x = 0;
        static float s_off_y = 0;
        tree.off_x = s_off_x;
        tree.off_y = s_off_y;
        tree.flags = UI_TREE_NODE_ICON;

        struct scope stk = {0};
        scope_begin(&stk, &app_tmp_arena);
        int *tmp = arena_push_array(&app_tmp_arena, int, app_tree_depth + 1);
        ui_tree_show_lines(&tree, tmp, app_tree_depth + 1);

        res_activate_icon(ctx->res, ICON_DIR);
        res_active_icon_dimension(ctx->res, ICON_DIM_SMALL);
        while (ui_tree_begin(ctx, &tree, pan)) {
            ui_fs_node_r(ctx, &tree, 0, app_tree_root);
            ui_tree_end(ctx, &tree);
        }
        scope_end(&stk);

        s_off_x = tree.off_x;
        s_off_y = tree.off_y;
    } ui_panel_end(pan);
    return tree.selection_changed;
}
/* ---------------------------------------------------------------------------
 *                              UI: Directory
 * --------------------------------------------------------------------------- */
static ui_bool
ui_dir_icon(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
     /* list each directory and file */
    ui_panel_begin(ctx, pan, root); {
        /* calculate icon dimensions */
        static const int margin = 8;
        const struct xfont *xf = res_font(ctx->res);
        const int icon_size = res_icon_dim_size(app_icon_size);

        /* select icon size */
        switch (app_icon_size) {
        case ICON_SIZE_SMALL:
            res_active_icon_dimension(ctx->res, ICON_DIM_SMALL); break;
        case ICON_SIZE_MEDIUM: default:
            res_active_icon_dimension(ctx->res, ICON_DIM_NORMAL); break;
        case ICON_SIZE_LARGE:
            res_active_icon_dimension(ctx->res, ICON_DIM_BIG); break;}

        struct ui_panel reg = {0};
        struct ui_area area = {.off_y = app_cur_dir_off_y};
        while (ui_area_begin(ctx, &area, &reg, pan)) {
            /* setup list */
            struct ui_list ls = {0};
            ls.orient = UI_HORIZONTAL;
            ls.flow = UI_FLOW_WRAP;
            ls.padx = ls.pady = 3;
            ls.spacing_x = 2;
            ls.spacing_y = 4;
            ls.col_width = 128;
            ls.row_height = xf->h + icon_size + margin;
            ui_list_setup(&ls, &reg);

            /* setup view */
            struct ui_view view = {0};
            view.offset = area.off_y;
            view.total = buf_cnt(app_cur_dir_content);
            ui_view_panel(&view, &ls, &reg);
            reg.max_y = view.max_y;

            /* directory content */
            for (int i = view.begin; i < view.end; ++i) {
                /* select icon */
                const struct file_info *fi = app_cur_dir_content + i;
                const int icon_id = cast(int, file_icon(fi->type));
                res_activate_icon(ctx->res, icon_id);

                /* button */
                struct ui_panel btn = {0};
                ui_push_ptr_id(&ctx->ids, fi);
                ui_list_gen(&btn.box, &ls);
                ui_desktop_icon(ctx, &btn, &reg, fi->name, 0);
                ui_pop_id(&ctx->ids);

                /* change directory */
                if (btn.doubled && fi->is_dir) {
                    struct directory *d = fi->owner;
                    fs_load_dir_at_path(fi->fullpath, &app_tree_arena);
                    for (int j = 0; j < buf_cnt(d->dirs); ++j) {
                        if (strcmp(fi->name, d->dirs[j]->name) == 0) {
                            d->ui.state = UI_TREE_NODE_EXPANDED;
                            dir_activate(d->dirs[j]);
                            undo_push(d->dirs[j]);
                            break;
                        }
                    } ui_area_end(ctx, &area);
                    return ui_true;
                }
            } ui_area_end(ctx, &area);
        } app_cur_dir_off_y = area.off_y;
    } ui_panel_end(pan);
    return ui_false;
}
static void
ui_report_header(struct ui_ctx *ctx, struct ui_panel *btn, struct ui_panel *parent,
    const char *begin, const char *end)
{
    ui_button_begin(ctx, btn, parent); {
        struct ui_panel lbl = {.unselectable = ui_true};
        struct extend ext = xf_text_measure(res_font(ctx->res), begin, end);
        ui_anchor_left_width(&lbl.box, btn->box.left + 8, ext.w);
        ui_anchor_center_height(&lbl.box, btn->box.center_y, ext.h);
        xs_color_foreground(ctx->sys->surf, xc_rgb(0,0,0));
        ui_label(ctx, &lbl, btn, begin, end);
    } ui_button_end(ctx, btn);
}
static ui_bool
ui_dir_report(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_panel_begin(ctx, pan, root); {
        /* columns */
        enum columns {NAME, TYPE, SIZE, MOD, COLCNT};
        static const char *colid[COLCNT] = {"Name", "Type", "Size", "Date Modified"};
        static int colw[COLCNT] = {150,150,150,150};
        typedef int(*sort_f)(const void *a, const void *b);
        static const sort_f colsort_f[] = {
            dir_compare_fileinfo_asc, dir_compare_fileinfo_desc,
            dir_compare_fileinfo_asc, dir_compare_fileinfo_desc,
            dir_compare_fileinfo_size_asc, dir_compare_fileinfo_size_desc,
            dir_compare_fileinfo_time_asc, dir_compare_fileinfo_time_desc
        };
        static int colsort = 0;
        static int colorder = 0;

        struct ui_panel reg = {0};
        struct ui_area area = {.off_y = app_cur_dir_off_y};
        while (ui_area_begin(ctx, &area, &reg, pan)) {
            static const int margin = 6;
            const struct xfont *xf = res_font(ctx->res);
            const int row_height = xf->h + margin;

            int at_x = reg.box.left + 2;
            for (int col = 0; col < COLCNT; ++col) {
                /* header */
                struct ui_panel hdr = {0};
                ui_anchor_left_width(&hdr.box, at_x, colw[col]);
                ui_anchor_top_height(&hdr.box, pan->box.top, row_height);
                ui_report_header(ctx, &hdr, &reg, colid[col], 0);
                if (hdr.clicked) {
                    if (colsort == col)
                        colorder = !colorder;
                    else colsort = col, colorder = 0;

                    qsort(app_cur_dir_content, (size_t)buf_cnt(app_cur_dir_content),
                        sizeof(struct file_info), colsort_f[(col << 1) + colorder]);
                    ui_area_end(ctx, &area);
                    return ui_true;
                }
                /* setup list */
                struct ui_list ls = {0};
                ls.orient = UI_VERTICAL;
                ls.spacing_y = 4;
                ls.padx = 2, ls.pady = 3;
                ls.col_width = colw[col];
                ls.row_height = row_height;
                ui_anchor_left_right(&ls.box, at_x, reg.box.right);
                ui_anchor_top_bottom(&ls.box, reg.box.top + row_height, reg.box.bottom);
                ui_list_init(&ls);

                /* setup view */
                struct ui_view view = {0};
                view.offset = area.off_y;
                view.total = buf_cnt(app_cur_dir_content);
                ui_view(&view, &ls, colw[col], reg.box.height - row_height);
                reg.max_y = view.max_y;

                /* list */
                struct scissor_rect old;
                old = ui_clip_begin(ctx, ls.at_x, hdr.box.bottom + 2, colw[col], view.space_y);
                for (int i = view.begin; i < view.end; ++i) {
                    struct file_info *fi = app_cur_dir_content + i;

                    /* select correct icon  */
                    const int icon_id = cast(int, file_icon(fi->type));
                    res_active_icon_dimension(ctx->res, ICON_DIM_SMALL);
                    res_activate_icon(ctx->res, icon_id);
                    if (fi->state != FILE_INFO_LOADED)
                        dir_stat_file(fi);

                    /* item */
                    struct ui_panel item = {0};
                    ui_push_ptr_id(&ctx->ids, fi);
                    ui_list_gen(&item.box, &ls);
                    xs_color_foreground(ctx->sys->surf, xc_rgb(0,0,0));

                    switch (col) {
                    default: assert(0); break;
                    case NAME: ui_icon_label(ctx, &item, &reg, fi->name, 0); break;
                    case SIZE: ui_labelf(ctx, &item, &reg, "%zu", fi->stat.st_size); break;
                    case TYPE: ui_label(ctx, &item, &reg, app_file_defs[fi->type].name, 0); break;
                    case MOD: ui_time(ctx, &item, &reg, "%d/%m/%Y %H:%M:%S", localtime(&fi->stat.st_mtim.tv_sec));}
                    ui_pop_id(&ctx->ids);

                    /* change directory */
                    if (item.doubled && fi->is_dir) {
                        struct directory *d = fi->owner;
                        fs_load_dir_at_path(fi->fullpath, &app_tree_arena);
                        for (int j = 0; j < buf_cnt(d->dirs); ++j) {
                            if (strcmp(fi->name, d->dirs[j]->name) == 0) {
                                d->ui.state = UI_TREE_NODE_EXPANDED;
                                dir_activate(d->dirs[j]);
                                undo_push(d->dirs[j]);
                                break;
                            }
                        } ui_area_end(ctx, &area);
                        return ui_true;
                    }
                }
                ui_clip_end(ctx, old);
                at_x += colw[col];
            }
            ui_area_end(ctx, &area);
        } app_cur_dir_off_y = area.off_y;
    }
    ui_panel_end(pan);
    return ui_false;
}
static ui_bool
ui_dir(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    switch (app_directory_view) {
    case DIR_VIEW_ICON:
        return ui_dir_icon(ctx, pan, root);
    case DIR_VIEW_REPORT:
        return ui_dir_report(ctx, pan, root);}
    return ui_false;
}

/* ---------------------------------------------------------------------------
 *                              UI: Filesystem
 * --------------------------------------------------------------------------- */
static ui_bool
ui_file_select_vertical(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_bool mod = ui_false;
    ui_panel_begin(ctx, pan, root);
    {
        static const int padx = 6;
        static const int pady = 6;
        static const int spacing = 4;
        static float ratio = 0.70f;
        float total = cast(float, pan->box.height - (spacing + 2 * pady));

        /* directory */
        struct ui_panel dir = {0};
        ui_anchor_left_right(&dir.box, pan->box.left + padx, pan->box.right - padx);
        ui_anchor_top_height(&dir.box, pan->box.top + pady, floori(total * ratio));
        mod = ui_dir(ctx, &dir, pan);

        /* separator */
        struct ui_panel sep = {0};
        ui_anchor_left_right(&sep.box, dir.box.left, dir.box.right);
        ui_anchor_top_height(&sep.box, dir.box.bottom, spacing);
        ui_panel(ctx, &sep, pan);
        if (sep.dragged) {
            ratio = ui_drag_ratio(ctx, UI_VERTICAL, sep.box.top, dir.box.top, total);
            ratio = clamp(0.1f, ratio, 0.9f);
            mod = ui_true;
        }
        /* tree */
        struct ui_panel tree = {0};
        ui_anchor_left_right(&tree.box, dir.box.left, dir.box.right);
        ui_anchor_top_bottom(&tree.box, sep.box.bottom, pan->box.bottom - pady);
        mod = ui_fs(ctx, &tree, pan) || mod;
    }
    ui_panel_end(pan);
    return mod;
}
static ui_bool
ui_file_select_horizontal(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_bool mod = ui_false;
    ui_panel_begin(ctx, pan, root);
    {
        static const int padx = 6;
        static const int pady = 6;
        static const int spacing = 4;
        static float ratio = 0.25f;
        float total = cast(float, pan->box.width - spacing - 2*pady);

        /* tree */
        struct ui_panel tree = {0};
        ui_anchor_left_width(&tree.box, pan->box.left + padx, floori(total * ratio));
        ui_anchor_top_bottom(&tree.box, pan->box.top + pady, pan->box.bottom - pady);
        mod = ui_fs(ctx, &tree, pan);

        /* separator */
        struct ui_panel sep = {0};
        ui_anchor_left_width(&sep.box, tree.box.right, spacing);
        ui_anchor_top_bottom(&sep.box, tree.box.top, tree.box.bottom);
        ui_panel(ctx, &sep, pan);
        if (sep.dragged) {
            ratio = ui_drag_ratio(ctx, UI_HORIZONTAL, sep.box.left, tree.box.left, total);
            ratio = clamp(0.1f, ratio, 0.9f);
            mod = ui_true;
        }
        /* directory */
        struct ui_panel dir = {0};
        ui_anchor_left_right(&dir.box, sep.box.right, pan->box.right - padx);
        ui_anchor_top_bottom(&dir.box, tree.box.top, tree.box.bottom);
        mod = ui_dir(ctx, &dir, pan) || mod;
    }
    ui_panel_end(pan);
    return mod;
}
static void
ui_file_select(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_panel_begin(ctx, pan, root); {
        struct ui_panel reg = {0};
        struct ui_area area = {0};
        area.type = UI_AREA_FIXED;
        area.style = UI_AREA_STYLE_FLAT;
        area.border_style = UI_BORDER_STYLE_NONE;

        while (ui_area_begin(ctx, &area, &reg, pan)) {
            ui_bool mod = ui_false;
            struct ui_panel content = {0};
            content.box = reg.box;

            switch (app_dir_layout) {
            case DIR_LAYOUT_VERTICAL:
                mod = ui_file_select_vertical(ctx, &content, &reg); break;
            case DIR_LAYOUT_HORIZONTAL:
                mod = ui_file_select_horizontal(ctx, &content, &reg); break;
            case DIR_LAYOUT_SWITCH: {
                if (pan->box.width < 700)
                    mod = ui_file_select_vertical(ctx, &content, &reg);
                else mod = ui_file_select_horizontal(ctx, &content, &reg);
            } break;}

            if (mod) area.state = UI_INCONSISTENT;
            ui_area_end(ctx, &area);
        }
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                              UI: Settings
 * --------------------------------------------------------------------------- */
static void
ui_property(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root,
    const struct var *var)
{
    static const int spacing = 4;
    ui_panel_begin(ctx, pan, root); {
        /* name */
        struct ui_panel lbl = {0};
        xs_color_foreground(ctx->sys->surf, xc_rgb(0,0,0));
        ui_anchor_left_width(&lbl.box, pan->box.left, (pan->box.width >> 1));
        ui_anchor_top_height(&lbl.box, pan->box.top, pan->box.height);
        ui_label(ctx, &lbl, pan, var->print, 0);

        struct ui_box box = {0};
        ui_anchor_left_width(&box, lbl.box.right + spacing, (pan->box.width >> 1));
        ui_anchor_top_height(&box, lbl.box.top, pan->box.height);

        /* value */
        switch (var->type) {
        case VAR_FLOAT: break;
        case VAR_INT: {
            struct ui_spinner spinner = {0};
            struct ui_panel spin = {.box = box};
            *var->val.i.val = ui_spinner(ctx, &spinner, &spin, root, *var->val.i.val);
        } break;
        case VAR_STRING: {
            struct ui_edit_box edit = {0};
            struct ui_panel txt = {.box = box};

            int len = (int)strlen(var->val.s.buf);
            ui_edit_box(ctx, &edit, &txt, root, var->val.s.buf, &len, var->val.s.len);
            var->val.s.buf[min(var->val.s.len - 1, len)] = '\0';
        } break;
        case VAR_ENUM: {
            struct ui_panel c = {.box = box};
            struct ui_combo com = {.selected = *var->val.e.sel};
            ui_combo(ctx, &com, &c, pan, var->val.e.options, var->val.e.num);
            *var->val.e.sel = com.selected;
        } break;}
    } ui_panel_end(pan);
}
static void
ui_settings(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_panel_begin(ctx, pan, root); {
        static const float offset_y = 0.0f;
        struct ui_area area = {.off_y = offset_y};
        area.padx = area.pady = 6;

        struct ui_panel reg = {0};
        while (ui_area_begin(ctx, &area, &reg, pan)) {
            static const int margin = 10;
            const struct xfont *xf = res_font(ctx->res);
            const int row_height = xf->h + margin;

            /* header */
            struct ui_panel nam;
            ui_anchor_left_width(&nam.box, reg.box.left + 2, (reg.box.width >> 1));
            ui_anchor_top_height(&nam.box, pan->box.top + 3, row_height);
            ui_report_header(ctx, &nam, &reg, "Name", 0);

            struct ui_panel val;
            ui_anchor_left_width(&val.box, nam.box.right, nam.box.width);
            ui_anchor_top_height(&val.box, nam.box.top, nam.box.height);
            ui_report_header(ctx, &val, &reg, "Value", 0);

            /* setup list */
            struct ui_list ls = {0};
            ls.orient = UI_VERTICAL;
            ls.padx = 8, ls.pady = 8;
            ls.spacing_y = 2;
            ls.row_height = row_height;
            ls.col_width = reg.box.width - (ls.padx << 1);
            ui_anchor_left_right(&ls.box, nam.box.left, reg.box.right);
            ui_anchor_top_bottom(&ls.box, nam.box.bottom, reg.box.bottom);
            ui_list_init(&ls);

            /* output each property */
            for (int i = 0; i < MAX_VAR_HASH; ++i) {
                const struct var *var = g_varsys.vars[i];
                if (!var) continue;

                struct ui_panel property = {0};
                ui_list_gen(&property.box, &ls);
                ui_property(ctx, &property, &reg, var);
            } ui_area_end(ctx, &area);
        }
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                              UI: Content
 * --------------------------------------------------------------------------- */
static void
ui_content(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *root)
{
    ui_panel_begin(ctx, pan, root); {
        static const char *titles[] = {"Files", "Settings"};
        enum tab_id {FILES, SETTINGS};
        static int s_active = FILES;

        struct ui_tab_control ctrl = {.selection = s_active};
        s_active = ui_tab_control(ctx, &ctrl, pan, titles, cntof(titles));
        switch (s_active) {
        default: assert(0); break;
        case FILES: ui_file_select(ctx, &ctrl.body, pan); break;
        case SETTINGS: ui_settings(ctx, &ctrl.body, pan); break;}
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *                                  UI: Window
 * --------------------------------------------------------------------------- */
static void
ui_window(struct ui_ctx *ctx, struct ui_panel *pan, struct ui_panel *parent)
{
    ui_panel_begin(ctx, pan, parent); {
        static const int spacing = 6;
        static const int menu_pad = 8;
        static const int main_pad = 5;
        static const int tool_pad = 4;

        const struct xfont *xf = res_font(ctx->res);
        const int small_icon_size = res_icon_dim_size(ICON_DIM_SMALL);
        const int norm_icon_size = res_icon_dim_size(ICON_DIM_NORMAL);

        /* menu */
        struct ui_panel menu = {0};
        const int menu_height = max(xf->h, small_icon_size) + menu_pad;
        res_active_icon_dimension(ctx->res, ICON_DIM_SMALL);
        ui_anchor_top_height(&menu.box, pan->box.top + main_pad, menu_height);
        ui_anchor_left_right(&menu.box, pan->box.left + main_pad,
            pan->box.right - main_pad);
        ui_menu(ctx, &menu, pan);

        /* tool */
        struct ui_panel tool = {0};
        const int tool_height = xf->h + norm_icon_size + tool_pad;
        res_active_icon_dimension(ctx->res, ICON_DIM_NORMAL);
        ui_anchor_top_height(&tool.box, menu.box.bottom + spacing, tool_height);
        ui_anchor_left_right(&tool.box, menu.box.left, menu.box.right);
        ui_tool(ctx, &tool, pan);

        /* content */
        struct ui_panel content = {0};
        ui_anchor_left_right(&content.box, tool.box.left, tool.box.right);
        ui_anchor_top_bottom(&content.box, tool.box.bottom + spacing, pan->box.bottom - main_pad);
        ui_content(ctx, &content, pan);
    } ui_panel_end(pan);
}

/* ---------------------------------------------------------------------------
 *
 *                                  MAIN
 *
 * --------------------------------------------------------------------------- */
#define DTIME           26
#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600

int main(int argc, char **argv)
{
    /* platform */
    struct sys sys = {0};
    sys.win.title = "Kaon";
    sys.win.w = WINDOW_WIDTH;
    sys.win.h = WINDOW_HEIGHT;
    sys.time.min_frame = DTIME;
    sys_init(&sys);

    /* con */
    cmd_init(&g_cmdsys);
    var_init(&g_varsys);

    /* resources */
    struct res res = {0};
    res_init(&res, &sys, &app_resources_def);

    /* ui */
    struct ui_ctx ctx = {0};
    ctx.sys = &sys;
    ctx.res = &res;

    /* filesystem */
    const char *home = getenv("HOME");
    fs_load_fullpath(home, &app_tree_arena);
    dir_load_files(app_dir_cur, &app_dir_arena);
    undo_push(app_dir_cur);

    /* var */
    var_insert(&g_varsys, &g_var_icon_size);
    var_insert(&g_varsys, &g_var_dir_view);
    var_insert(&g_varsys, &g_var_dir_layout);
    cmd_exec("test.conf");

    /* application */
    while (!sys.quit) {
        sys_poll(&sys);
        xs_clear(sys.surf, xc_rgb(212,208,200));

        ui_begin(&ctx);
        struct ui_panel win = {.box = ctx.root.box};
        ui_window(&ctx, &win, &ctx.root);
        ui_end(&ctx);

        sys_push(&sys);
    }
    arena_free(&app_tree_arena);
    arena_free(&app_tmp_arena);
    arena_free(&app_dir_arena);
    buf_free(app_cur_dir_content);

    /* shutdown */
    res_shutdown(&res, &sys);
    sys_shutdown(&sys);
    return 0;
}
