typedef int ui_bool;
enum {ui_false, ui_true};

/* Library */
struct ui_id {uintptr_t lo, hi;};
struct ui_box {
    int left, top, right, bottom;
    int width, height;
    int center_x, center_y;
};
enum ui_state {
    UI_NORMAL,
    UI_FOCUSED,
    UI_DISABLED,
};
struct ui_panel {
    struct ui_panel *parent;
    struct ui_id id;
    struct ui_box box;
    int max_x, max_y;

    enum ui_state state;
    unsigned unselectable:1;
    unsigned focusable:1;

    /* state */
    unsigned doubled:1;
    unsigned hovered:1;
    unsigned entered:1;
    unsigned exited:1;
    unsigned clicked:1;
    unsigned pressed:1;
    unsigned down:1;
    unsigned released:1;
    unsigned drag_begin:1;
    unsigned dragged:1;
    unsigned drag_end:1;
    unsigned scrolled:1;
};
struct ui_id_stk {
    int top;
    #define UI_ID_STACK_DEPTH  256
    struct ui_id stk[UI_ID_STACK_DEPTH];
};
enum ui_layout {
    UI_LAYOUT_STRETCH,
    UI_LAYOUT_FIT
};
enum ui_popup_type {
    UI_POPUP_BLOCKING,
    UI_POPUP_NON_BLOCKING,
};
struct ui_popup {
    ui_bool active;
    enum ui_popup_type type;
    int x,y,w,h;
    struct scissor_rect clip;
    struct ui_id id;
    struct xsurface *surf;
    struct xsurface *old;
    unsigned seq;
};
struct ui_buffer {
    struct ui_id id;
    ui_bool active;
    char buf[8*1024];
    int len;
};
struct ui_ctx {
    /* systems */
    struct sys *sys;
    struct res *res;
    /* state */
    struct ui_id_stk ids;
    enum ui_layout layout;
    struct scissor_rect clip;
    struct ui_popup popup;
    unsigned disabled:1;
    unsigned activate_next:1;
    unsigned seq;
    /* tree */
    struct ui_panel root;
    struct ui_id active;
    struct ui_id origin;
    struct ui_id hot;
};

/* Util: Record */
struct ui_record {
    struct ui_ctx *ctx;
    struct ui_panel *pan;
    int max_x, max_y;
    struct ui_id id_begin;
};

/* Widget: Scroll */
struct ui_scroll {
    struct ui_panel pan;
    float total_x, total_y; /* in */
    float size_x, size_y;   /* in */
    float off_x, off_y;     /* in-out */
    unsigned scrolled:1;    /* out */
};
struct ui_scrollbar {
    float total;            /* in */
    float size;             /* in */
    float off;              /* in-out */
    unsigned scrolled:1;    /* out */
};

/* Widget: Area */
enum ui_widget_state {
    UI_INCONSISTENT,
    UI_CONSISTENT
};
enum ui_border_style {
    UI_BORDER_STYLE_3D,
    UI_BORDER_STYLE_SINGLE,
    UI_BORDER_STYLE_NONE
};
enum ui_area_style {
    UI_AREA_STYLE_SUNKEN,
    UI_AREA_STYLE_FLAT
};
enum ui_area_type {
    UI_AREA_SCROLL,
    UI_AREA_FIXED
};
struct ui_area {
    struct ui_panel *pan;
    enum ui_area_type type;
    enum ui_widget_state state;
    enum ui_border_style border_style;
    enum ui_area_style style;

    float off_x, off_y;
    float max_offx, max_offy;
    int padx, pady;
    unsigned scrolled:1;

    struct ui_record record;
    struct scissor_rect clip_rect;
    unsigned background;
};

/* Widget: Group Box */
enum ui_group_box_flow {
    UI_GROUP_BOX_STRETCH,
    UI_GROUP_BOX_FIT_WIDTH = 0x01,
    UI_GROUP_BOX_FIT_HEIGHT = 0x02,
    UI_GROUP_BOX_FIT = UI_GROUP_BOX_FIT_WIDTH|UI_GROUP_BOX_FIT_HEIGHT
};
struct ui_group_box {
    struct ui_panel *pan;
    enum ui_group_box_flow flow;
    int title_right;
    int title_pad;
    int top;
};

/* Widget: Radio Group */
struct ui_radio_group {
    enum ui_widget_state state;
    struct ui_record record;
    int idx, toggled;
    int selected; /* in-out */
};

/* Widget: Edit */
enum ui_edit_box_type {
    UI_EDIT_BOX_DEFAULT,
    UI_EDIT_BOX_INT,
    UI_EDIT_BOX_REAL,
    UI_EDIT_BOX_HEX,
    UI_EDIT_BOX_BIN,
    UI_EDIT_BOX_MAX
};
typedef int(*ui_filter_f)(long rune);
struct ui_edit_box {
    enum ui_edit_box_type type;
    ui_filter_f filter;
    unsigned state;
};

/* Widget: Spinner */
struct ui_spinner {
    int min, inc, max;
};

/* Widget: Combo */
enum ui_combo_stage {
    UI_COMBO_HEADER,
    UI_COMBO_LAYOUT,
    UI_COMBO_EXEC,
    UI_COMBO_DONE
};
enum ui_combo_state {
    UI_COMBO_COLLAPSED,
    UI_COMBO_EXPANDED,
};
struct ui_combo {
    /* processing */
    enum ui_combo_stage stage;
    struct ui_panel *pan;
    struct ui_panel popup;
    struct ui_area area;
    struct ui_panel area_panel;
    struct ui_record record;
    int at_y, idx;
    /* parameter */
    int max_height;
    int selected;
    int off;
    /* result */
    enum ui_combo_state state;
    unsigned selection_changed:1;
};

/* Widget: Tab Control */
enum ui_tab_control_stage {
    UI_TAB_LAYOUT,
    UI_TAB_INPUT,
    UI_TAB_RENDER,
    UI_TAB_DONE
};
struct ui_tab_control {
    struct ui_panel body;   /* out */
    int selection;          /* in-out */
    ui_bool toggled;        /* out */
    int idx;

    /* internal */
    struct ui_panel *pan;
    unsigned activate_next:1;
    enum ui_tab_control_stage stage;
    struct ui_panel *parent;
    struct ui_record rec;
    int fixed_size;
    int at_x, at_y;
    int off_x;
};

/* Utility: List */
enum ui_orientation {
    UI_HORIZONTAL,
    UI_VERTICAL,
};
enum ui_flow {
    UI_FLOW_STRAIGHT,
    UI_FLOW_WRAP
};
struct ui_list {
    enum ui_orientation orient;
    enum ui_flow flow;
    struct ui_box box;
    int padx, pady;
    int spacing_x;
    int spacing_y;
    int col_width;
    int row_height;
    /* internal */
    int at_x, at_y;
};

/* Utility: View */
struct ui_view {
    /* in */
    float offset;
    int total;

    /* out */
    int begin, end;
    int space_x;
    int space_y;
    int col_width;
    int row_height;
    int col_cnt;
    int row_cnt;
    int total_height;
    int total_width;
    int max_x;
    int max_y;

    /* internal */
    enum ui_orientation orient;
    enum ui_flow flow;
};

/* Widget: Tree */
enum ui_tree_node_type {
    UI_TREE_NODE_INTERNAL,
    UI_TREE_NODE_LEAF
};
enum ui_tree_node_state {
    UI_TREE_NODE_COLLAPSED,
    UI_TREE_NODE_EXPANDED,
};
enum ui_tree_node_flags {
    UI_TREE_NODE_ICON   = 0x01,
    UI_TREE_NODE_CHECK  = 0x02,
};
struct ui_tree_node {
    /* parameter */
    unsigned type:1;
    unsigned state:1;
    /* state */
    unsigned checked:1;
    unsigned selected:1;
    unsigned check_changed:1;
    unsigned selection_changed:1;
    unsigned state_changed:1;
};
struct ui_tree {
    struct ui_area area;
    struct ui_panel area_panel;
    int depth_off;

    /* in */
    float off_x, off_y;
    unsigned flags;

    /* state */
    int at_y, idx;
    struct ui_id selected_node;
    unsigned inconsistent:1;
    unsigned repaint:1;
    unsigned selection_changed:1;
    unsigned focus_next:1;

    /* line drawing */
    unsigned show_lines:1;
    struct ui_id prev;
    int prev_depth;
    int prev_state;
    int *y_at_depth;
    int max_depth;
};
