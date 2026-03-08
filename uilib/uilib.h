#ifndef UILIB_H
#define UILIB_H

#include "../include/prism.h"
#include <stddef.h>

/* =============================================================================
 * Core types (alias / wrap Prism)
 * ============================================================================= */
typedef position_t uilib_point_t;
typedef prism_size_t uilib_size_t;
typedef rect_t uilib_rectangle_t;
typedef color_t uilib_color_t;

#define uilib_point(X, Y) ((uilib_point_t){ (X), (Y) })
#define uilib_size(W, H) ((uilib_size_t){ (W), (H) })
#define uilib_rect(X, Y, W, H) ((uilib_rectangle_t){ { (X), (Y) }, { (W), (H) } })

/* =============================================================================
 * EventArgs
 * ============================================================================= */
typedef struct uilib_event_args { int _reserved; } uilib_event_args_t;

typedef struct uilib_mouse_event_args {
    uilib_event_args_t base;
    int x, y;
    int button;  /* 0=left, 1=right, 2=middle */
    bool pressed;
} uilib_mouse_event_args_t;

typedef struct uilib_key_event_args {
    uilib_event_args_t base;
    key_t key;
    bool pressed;
} uilib_key_event_args_t;

typedef struct uilib_paint_event_args {
    uilib_event_args_t base;
    window_t *window;
    uilib_rectangle_t clip_rect;
} uilib_paint_event_args_t;

/* =============================================================================
 * Font, Brush, Pen (wrappers)
 * ============================================================================= */
typedef struct uilib_font {
    int size;  /* scale for Prism built-in font */
} uilib_font_t;

typedef struct uilib_brush {
    uilib_color_t color;
} uilib_brush_t;

typedef struct uilib_pen {
    uilib_color_t color;
    int width;
} uilib_pen_t;

#define UILIB_FONT_DEFAULT { 1 }
#define UILIB_BRUSH(C) { (C) }
#define UILIB_PEN(C, W) { (C), (W) }

/* =============================================================================
 * Image, Icon (Image uses Prism image_t; Icon is placeholder)
 * ============================================================================= */
typedef image_t uilib_image_t;
typedef struct uilib_icon {
    uilib_image_t image;
} uilib_icon_t;

/* =============================================================================
 * Cursor (placeholder - no native cursor support without platform code)
 * ============================================================================= */
typedef enum {
    UILIB_CURSOR_DEFAULT,
    UILIB_CURSOR_HAND,
    UILIB_CURSOR_TEXT,
} uilib_cursor_type_t;

/* =============================================================================
 * Theme and context
 * ============================================================================= */
typedef struct uilib_theme {
    uilib_color_t background;
    uilib_color_t panel;
    uilib_color_t button;
    uilib_color_t button_hover;
    uilib_color_t button_press;
    uilib_color_t border;
    uilib_color_t text;
    uilib_color_t text_dim;
    uilib_color_t selected;
    uilib_color_t checkbox;
} uilib_theme_t;

extern const uilib_theme_t UILIB_THEME_DARK;

typedef struct uilib_ctx {
    window_t *window;
    mouse_t mouse;
    mouse_t prev_mouse;
    const uilib_theme_t *theme;
    uilib_rectangle_t clip;       /* current clip region */
    uilib_point_t scroll_offset;  /* for ScrollViewer */
    unsigned int hot_id;          /* widget under mouse */
    unsigned int active_id;       /* widget being interacted with */
    int last_key_char;            /* for TextBox input */
} uilib_ctx_t;

/* =============================================================================
 * Control base (conceptual - used by layout helpers)
 * ============================================================================= */
typedef struct uilib_control {
    uilib_rectangle_t bounds;
    bool visible;
    bool enabled;
} uilib_control_t;

/* =============================================================================
 * Application and Window
 * ============================================================================= */
typedef struct uilib_application {
    window_t *main_window;
    int exit_code;
} uilib_application_t;

typedef struct uilib_window {
    window_t *handle;
    uilib_rectangle_t client_bounds;
} uilib_window_t;

uilib_application_t uilib_app_create(prism_size_t size, uilib_point_t pos);
void uilib_app_run(uilib_application_t *app);
void uilib_app_exit(uilib_application_t *app, int code);

/* =============================================================================
 * Timer
 * ============================================================================= */
typedef struct uilib_timer {
    unsigned int interval_ms;
    unsigned int last_tick;
    bool running;
    void (*on_tick)(struct uilib_timer *t, void *user);
    void *user_data;
} uilib_timer_t;

void uilib_timer_start(uilib_timer_t *t, unsigned int interval_ms);
void uilib_timer_stop(uilib_timer_t *t);
void uilib_timer_tick(uilib_timer_t *t, unsigned int now_ms);

/* =============================================================================
 * Frame begin
 * ============================================================================= */
void uilib_begin(uilib_ctx_t *ctx, window_t *window, const uilib_theme_t *theme);

/* =============================================================================
 * Layout panels - compute child rects
 * ============================================================================= */
typedef enum { UILIB_ORIENT_VERT, UILIB_ORIENT_HORZ } uilib_orient_t;
typedef enum { UILIB_DOCK_TOP, UILIB_DOCK_BOTTOM, UILIB_DOCK_LEFT, UILIB_DOCK_RIGHT, UILIB_DOCK_FILL } uilib_dock_t;

void uilib_stack_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, uilib_orient_t orient, int gap);
bool uilib_stack_next(uilib_ctx_t *ctx, int height_or_width, uilib_rectangle_t *out);

void uilib_grid_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, int cols, int rows);
void uilib_grid_cell(uilib_ctx_t *ctx, int col, int row, uilib_rectangle_t *out);

void uilib_dock_begin(uilib_ctx_t *ctx, uilib_rectangle_t container);
void uilib_dock_add(uilib_ctx_t *ctx, uilib_dock_t dock, int size, uilib_rectangle_t *out);

void uilib_wrap_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, int item_width, int item_height, int gap);
bool uilib_wrap_next(uilib_ctx_t *ctx, uilib_rectangle_t *out);

/* =============================================================================
 * Containers
 * ============================================================================= */
void uilib_panel(uilib_ctx_t *ctx, uilib_rectangle_t rect);
void uilib_group_box(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *title);

typedef struct uilib_scroll_state {
    int scroll_y;
    int content_height;
    int view_height;
} uilib_scroll_state_t;

void uilib_scroll_viewer_begin(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_scroll_state_t *state);
void uilib_scroll_viewer_end(uilib_ctx_t *ctx);

/* =============================================================================
 * Basic widgets
 * ============================================================================= */
bool uilib_button(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label);
void uilib_label(uilib_ctx_t *ctx, uilib_point_t pos, const char *text, bool dim);

typedef struct uilib_checkbox_state { bool checked; } uilib_checkbox_state_t;
bool uilib_checkbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label, uilib_checkbox_state_t *state);

typedef struct uilib_radiobutton_state { int selected; } uilib_radiobutton_state_t;
bool uilib_radiobutton(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label, int value, uilib_radiobutton_state_t *state);

/* =============================================================================
 * TextBox
 * ============================================================================= */
#define UILIB_TEXTBOX_MAX 256
typedef struct uilib_textbox_state {
    char buf[UILIB_TEXTBOX_MAX];
    int len;
    int caret;
    int scroll_x;
} uilib_textbox_state_t;

void uilib_textbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_textbox_state_t *state);

/* =============================================================================
 * ListBox, ComboBox
 * ============================================================================= */
#define UILIB_LIST_MAX 64
typedef struct uilib_listbox_state {
    const char *items[UILIB_LIST_MAX];
    int count;
    int selected;
    int scroll_offset;
} uilib_listbox_state_t;

bool uilib_listbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_listbox_state_t *state);

typedef struct uilib_combobox_state {
    const char *items[UILIB_LIST_MAX];
    int count;
    int selected;
    bool dropped;
} uilib_combobox_state_t;

bool uilib_combobox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_combobox_state_t *state);

/* =============================================================================
 * ScrollBar, ProgressBar, Slider
 * ============================================================================= */
typedef struct uilib_scrollbar_state {
    int value;
    int min_val;
    int max_val;
    int page_size;
} uilib_scrollbar_state_t;

bool uilib_scrollbar(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_scrollbar_state_t *state);

typedef struct uilib_progressbar_state {
    float value;  /* 0..1 */
} uilib_progressbar_state_t;

void uilib_progressbar(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_progressbar_state_t *state);

typedef struct uilib_slider_state {
    float value;  /* 0..1 */
} uilib_slider_state_t;

bool uilib_slider(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_slider_state_t *state);

/* =============================================================================
 * PictureBox
 * ============================================================================= */
void uilib_picture_box(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_image_t *img);

/* =============================================================================
 * TabControl, Splitter
 * ============================================================================= */
#define UILIB_TABS_MAX 16
typedef struct uilib_tab_state {
    const char *tabs[UILIB_TABS_MAX];
    int count;
    int selected;
} uilib_tab_state_t;

void uilib_tab_control_begin(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_tab_state_t *state);
void uilib_tab_control_end(uilib_ctx_t *ctx);

typedef struct uilib_splitter_state {
    int pos;   /* split position */
    bool dragging;
} uilib_splitter_state_t;

void uilib_splitter(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_splitter_state_t *state);

/* =============================================================================
 * Menus (custom-drawn, no native menu bar)
 * ============================================================================= */
#define UILIB_MENU_ITEMS_MAX 32
typedef struct uilib_menu_item {
    const char *label;
    int id;
    bool enabled;
    bool checked;
} uilib_menu_item_t;

typedef struct uilib_menu {
    uilib_menu_item_t items[UILIB_MENU_ITEMS_MAX];
    int count;
} uilib_menu_t;

typedef struct uilib_menu_strip {
    uilib_menu_t menus[UILIB_MENU_ITEMS_MAX];
    const char *menu_labels[UILIB_MENU_ITEMS_MAX];
    int count;
} uilib_menu_strip_t;

bool uilib_menu_strip(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_menu_strip_t *strip, int *out_id);

typedef struct uilib_context_menu {
    uilib_menu_item_t items[UILIB_MENU_ITEMS_MAX];
    int count;
    uilib_point_t position;
    bool open;
} uilib_context_menu_t;

bool uilib_context_menu(uilib_ctx_t *ctx, uilib_context_menu_t *menu, int *out_id);
void uilib_status_strip(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *text);

/* =============================================================================
 * Dialogs (platform-dependent; stubs on unsupported platforms)
 * ============================================================================= */
typedef enum {
    UILIB_DIALOG_OK,
    UILIB_DIALOG_OK_CANCEL,
    UILIB_DIALOG_YES_NO,
} uilib_message_buttons_t;

typedef enum {
    UILIB_RESULT_NONE,
    UILIB_RESULT_OK,
    UILIB_RESULT_CANCEL,
    UILIB_RESULT_YES,
    UILIB_RESULT_NO,
} uilib_dialog_result_t;

uilib_dialog_result_t uilib_message_box(const char *title, const char *message, uilib_message_buttons_t buttons);

#define UILIB_FILENAME_MAX 512
typedef struct uilib_open_file_dialog {
    char filename[UILIB_FILENAME_MAX];
    const char *filter;
} uilib_open_file_dialog_t;

bool uilib_open_file_dialog(uilib_open_file_dialog_t *d);

typedef struct uilib_save_file_dialog {
    char filename[UILIB_FILENAME_MAX];
    const char *filter;
} uilib_save_file_dialog_t;

bool uilib_save_file_dialog(uilib_save_file_dialog_t *d);

typedef struct uilib_folder_dialog {
    char path[UILIB_FILENAME_MAX];
} uilib_folder_dialog_t;

bool uilib_folder_browser_dialog(uilib_folder_dialog_t *d);

typedef struct uilib_color_dialog {
    uilib_color_t color;
} uilib_color_dialog_t;

bool uilib_color_dialog(uilib_color_dialog_t *d);

typedef struct uilib_font_dialog {
    uilib_font_t font;
} uilib_font_dialog_t;

bool uilib_font_dialog(uilib_font_dialog_t *d);

/* =============================================================================
 * PropertyGrid (simplified - fixed property list)
 * ============================================================================= */
#define UILIB_PROP_MAX 32
typedef enum { UILIB_PROP_INT, UILIB_PROP_FLOAT, UILIB_PROP_BOOL, UILIB_PROP_STRING } uilib_prop_type_t;

typedef struct uilib_property {
    const char *name;
    uilib_prop_type_t type;
    union {
        int i;
        float f;
        bool b;
        char s[64];
    } value;
} uilib_property_t;

typedef struct uilib_property_grid {
    uilib_property_t props[UILIB_PROP_MAX];
    int count;
} uilib_property_grid_t;

void uilib_property_grid(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_property_grid_t *grid);

#endif /* UILIB_H */
