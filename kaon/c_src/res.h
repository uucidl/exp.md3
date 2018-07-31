enum icon_dimensions {
    ICON_DIM_SMALL,
    ICON_DIM_NORMAL,
    ICON_DIM_BIG,
    ICON_DIM_COUNT
};
struct icon_def {int dim[ICON_DIM_COUNT];};
struct resoures_def {
    const struct ximage_def *img;
    int img_cnt;
    const struct icon_def *ico;
    int ico_cnt;
    const char **fnt;
    int fnt_cnt;
};
enum res_source {
    RES_SOURCE_ICON,
    RES_SOURCE_IMG,
};
struct res {
    const struct resoures_def *def;
    struct xsurface **imgs;
    int img_cnt;
    struct xfont **fnts;
    int fnt_cnt;

    /* runtime */
    enum res_source src;
    const struct xsurface* cur_img;
    struct xfont *cur_fnt;
    const struct icon_def *cur_icon;
    enum icon_dimensions cur_icon_dim;
};
static void res_init(struct res *res, struct sys *sys, const struct resoures_def *defs);
static void res_shutdown(struct res *res, struct sys *sys);
static void res_activate_icon(struct res *res, int id);
static void res_activate_font(struct res *res, int id);
static void res_activate_img(struct res *res, int id);
static void res_active_icon_dimension(struct res *res, enum icon_dimensions dim);
static int res_icon_dim_size(int dim);
static const struct xfont *res_font(const struct res *res);
