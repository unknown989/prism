#include "uilib.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(PRISM_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <commdlg.h>
#endif

/* -----------------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------------- */
static int in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

static void draw_text_at(window_t *w, int x, int y, const char *s, color_t c, int scale) {
    text_t t = { s, 0, 0 };
    position_t pos = { x, y };
    draw_text(w, t, c, pos, scale > 0 ? scale : 1);
}

/* -----------------------------------------------------------------------------
 * Layout state (internal)
 * ----------------------------------------------------------------------------- */
static struct {
    uilib_rectangle_t container;
    uilib_orient_t orient;
    int gap;
    int offset;
} stack_state;

static struct {
    uilib_rectangle_t container;
    int cols, rows;
    int cell_w, cell_h;
} grid_state;

static struct {
    uilib_rectangle_t container;
    int item_w, item_h;
    int gap;
    int x, y;
} wrap_state;

/* -----------------------------------------------------------------------------
 * Theme
 * ----------------------------------------------------------------------------- */
const uilib_theme_t UILIB_THEME_DARK = {
    .background   = { 28, 28, 32, 255 },
    .panel        = { 48, 48, 56, 255 },
    .button       = { 64, 64, 80, 255 },
    .button_hover = { 80, 80, 100, 255 },
    .button_press = { 56, 56, 72, 255 },
    .border       = { 90, 90, 100, 255 },
    .text         = { 240, 240, 245, 255 },
    .text_dim     = { 140, 140, 150, 255 },
    .selected     = { 64, 100, 164, 255 },
    .checkbox     = { 72, 72, 88, 255 },
};

/* -----------------------------------------------------------------------------
 * Begin
 * ----------------------------------------------------------------------------- */
void uilib_begin(uilib_ctx_t *ctx, window_t *window, const uilib_theme_t *theme) {
    if (!ctx || !window) return;
    ctx->prev_mouse = ctx->mouse;
    ctx->mouse = get_mouse(window);
    ctx->window = window;
    ctx->theme = theme ? theme : &UILIB_THEME_DARK;
    ctx->clip = (uilib_rectangle_t){ { 0, 0 }, { window->width, window->height } };
    ctx->scroll_offset = (uilib_point_t){ 0, 0 };
}

/* -----------------------------------------------------------------------------
 * Application / Window (minimal)
 * ----------------------------------------------------------------------------- */
uilib_application_t uilib_app_create(prism_size_t size, uilib_point_t pos) {
    uilib_application_t app = { 0 };
    app.main_window = create_window(size, pos, 0);
    return app;
}

void uilib_app_run(uilib_application_t *app) {
    (void)app;
}

void uilib_app_exit(uilib_application_t *app, int code) {
    if (app) app->exit_code = code;
}

/* -----------------------------------------------------------------------------
 * Timer
 * ----------------------------------------------------------------------------- */
void uilib_timer_start(uilib_timer_t *t, unsigned int interval_ms) {
    if (t) {
        t->interval_ms = interval_ms;
        t->running = true;
    }
}

void uilib_timer_stop(uilib_timer_t *t) {
    if (t) t->running = false;
}

void uilib_timer_tick(uilib_timer_t *t, unsigned int now_ms) {
    if (!t || !t->running) return;
    if (now_ms - t->last_tick >= t->interval_ms) {
        t->last_tick = now_ms;
        if (t->on_tick) t->on_tick(t, t->user_data);
    }
}

/* -----------------------------------------------------------------------------
 * Layout: StackPanel
 * ----------------------------------------------------------------------------- */
void uilib_stack_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, uilib_orient_t orient, int gap) {
    (void)ctx;
    stack_state.container = container;
    stack_state.orient = orient;
    stack_state.gap = gap;
    stack_state.offset = 0;
}

bool uilib_stack_next(uilib_ctx_t *ctx, int height_or_width, uilib_rectangle_t *out) {
    (void)ctx;
    int x = stack_state.container.position.x;
    int y = stack_state.container.position.y;
    int w = stack_state.container.size.width;
    int h = stack_state.container.size.height;
    int off = stack_state.offset;

    if (stack_state.orient == UILIB_ORIENT_VERT) {
        if (off + height_or_width > h) return false;
        *out = uilib_rect(x, y + off, w, height_or_width);
        stack_state.offset = off + height_or_width + stack_state.gap;
    } else {
        if (off + height_or_width > w) return false;
        *out = uilib_rect(x + off, y, height_or_width, h);
        stack_state.offset = off + height_or_width + stack_state.gap;
    }
    return true;
}

/* -----------------------------------------------------------------------------
 * Layout: Grid
 * ----------------------------------------------------------------------------- */
void uilib_grid_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, int cols, int rows) {
    (void)ctx;
    grid_state.container = container;
    grid_state.cols = cols > 0 ? cols : 1;
    grid_state.rows = rows > 0 ? rows : 1;
    grid_state.cell_w = container.size.width / grid_state.cols;
    grid_state.cell_h = container.size.height / grid_state.rows;
}

void uilib_grid_cell(uilib_ctx_t *ctx, int col, int row, uilib_rectangle_t *out) {
    (void)ctx;
    int x = grid_state.container.position.x + col * grid_state.cell_w;
    int y = grid_state.container.position.y + row * grid_state.cell_h;
    *out = uilib_rect(x, y, grid_state.cell_w, grid_state.cell_h);
}

/* -----------------------------------------------------------------------------
 * Layout: DockPanel (simplified - single dock per call)
 * ----------------------------------------------------------------------------- */
static uilib_rectangle_t dock_remaining;

void uilib_dock_begin(uilib_ctx_t *ctx, uilib_rectangle_t container) {
    (void)ctx;
    dock_remaining = container;
}

void uilib_dock_add(uilib_ctx_t *ctx, uilib_dock_t dock, int size, uilib_rectangle_t *out) {
    (void)ctx;
    int x = dock_remaining.position.x, y = dock_remaining.position.y;
    int w = dock_remaining.size.width, h = dock_remaining.size.height;
    if (size <= 0) size = 40;

    switch (dock) {
        case UILIB_DOCK_TOP:
            *out = uilib_rect(x, y, w, size);
            dock_remaining.position.y += size;
            dock_remaining.size.height -= size;
            break;
        case UILIB_DOCK_BOTTOM:
            *out = uilib_rect(x, y + h - size, w, size);
            dock_remaining.size.height -= size;
            break;
        case UILIB_DOCK_LEFT:
            *out = uilib_rect(x, y, size, h);
            dock_remaining.position.x += size;
            dock_remaining.size.width -= size;
            break;
        case UILIB_DOCK_RIGHT:
            *out = uilib_rect(x + w - size, y, size, h);
            dock_remaining.size.width -= size;
            break;
        case UILIB_DOCK_FILL:
            *out = dock_remaining;
            break;
    }
}

/* -----------------------------------------------------------------------------
 * Layout: WrapPanel
 * ----------------------------------------------------------------------------- */
void uilib_wrap_begin(uilib_ctx_t *ctx, uilib_rectangle_t container, int item_w, int item_h, int gap) {
    (void)ctx;
    wrap_state.container = container;
    wrap_state.item_w = item_w;
    wrap_state.item_h = item_h;
    wrap_state.gap = gap;
    wrap_state.x = container.position.x;
    wrap_state.y = container.position.y;
}

bool uilib_wrap_next(uilib_ctx_t *ctx, uilib_rectangle_t *out) {
    (void)ctx;
    int max_x = wrap_state.container.position.x + wrap_state.container.size.width;
    if (wrap_state.x + wrap_state.item_w > max_x) {
        wrap_state.x = wrap_state.container.position.x;
        wrap_state.y += wrap_state.item_h + wrap_state.gap;
    }
    if (wrap_state.y + wrap_state.item_h > wrap_state.container.position.y + wrap_state.container.size.height)
        return false;
    *out = uilib_rect(wrap_state.x, wrap_state.y, wrap_state.item_w, wrap_state.item_h);
    wrap_state.x += wrap_state.item_w + wrap_state.gap;
    return true;
}

/* -----------------------------------------------------------------------------
 * Containers
 * ----------------------------------------------------------------------------- */
void uilib_panel(uilib_ctx_t *ctx, uilib_rectangle_t rect) {
    if (!ctx || !ctx->window || !ctx->theme) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
}

void uilib_group_box(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *title) {
    if (!ctx || !ctx->window || !ctx->theme) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    if (title && title[0]) {
        draw_text_at(ctx->window, rect.position.x + 8, rect.position.y - 6,
            title, ctx->theme->text, 1);
    }
}

void uilib_scroll_viewer_begin(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_scroll_state_t *state) {
    if (!ctx || !state) return;
    ctx->clip = rect;
    ctx->scroll_offset.y = -state->scroll_y;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    if (state->content_height > state->view_height) {
        int sb_h = rect.size.height;
        int thumb_h = sb_h * state->view_height / state->content_height;
        if (thumb_h < 20) thumb_h = 20;
        int track = sb_h - thumb_h;
        int max_scroll = state->content_height - state->view_height;
        int sb_x = rect.position.x + rect.size.width - 16;
        int inside = in_rect(ctx->mouse.x, ctx->mouse.y, sb_x, rect.position.y, 16, sb_h);
        if (inside && ctx->mouse.left && !ctx->prev_mouse.left) {
            int rel = ctx->mouse.y - rect.position.y;
            state->scroll_y = (rel - thumb_h/2) * max_scroll / track;
            if (state->scroll_y < 0) state->scroll_y = 0;
            if (state->scroll_y > max_scroll) state->scroll_y = max_scroll;
        }
    }
}

void uilib_scroll_viewer_end(uilib_ctx_t *ctx) {
    if (ctx) {
        ctx->clip = (uilib_rectangle_t){ { 0, 0 }, { ctx->window->width, ctx->window->height } };
        ctx->scroll_offset.y = 0;
    }
}

/* -----------------------------------------------------------------------------
 * Button, Label
 * ----------------------------------------------------------------------------- */
bool uilib_button(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label) {
    if (!ctx || !ctx->window || !ctx->theme) return false;
    int rx = rect.position.x, ry = rect.position.y;
    int rw = rect.size.width, rh = rect.size.height;
    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, rx, ry, rw, rh);

    color_t fill = ctx->theme->button;
    if (inside && ctx->mouse.left) fill = ctx->theme->button_press;
    else if (inside) fill = ctx->theme->button_hover;

    fill_rect(ctx->window, rect, fill);
    draw_rect(ctx->window, rect, ctx->theme->border);
    if (label && label[0])
        draw_text_at(ctx->window, rx + 8, ry + (rh - 10) / 2, label, ctx->theme->text, 1);

    return inside && ctx->prev_mouse.left && !ctx->mouse.left;
}

void uilib_label(uilib_ctx_t *ctx, uilib_point_t pos, const char *text, bool dim) {
    if (!ctx || !ctx->window || !ctx->theme || !text) return;
    color_t c = dim ? ctx->theme->text_dim : ctx->theme->text;
    draw_text_at(ctx->window, pos.x, pos.y, text, c, 1);
}

/* -----------------------------------------------------------------------------
 * CheckBox, RadioButton
 * ----------------------------------------------------------------------------- */
bool uilib_checkbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label, uilib_checkbox_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    int box_size = 16;
    rect_t box = { { rect.position.x, rect.position.y + (rect.size.height - box_size) / 2 },
                   { box_size, box_size } };
    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, box.position.x, box.position.y, box.size.width, box.size.height);

    fill_rect(ctx->window, box, ctx->theme->checkbox);
    draw_rect(ctx->window, box, ctx->theme->border);
    if (state->checked) {
        rect_t check = { { box.position.x + 4, box.position.y + 4 }, { 8, 8 } };
        fill_rect(ctx->window, check, ctx->theme->text);
    }
    if (label && label[0])
        draw_text_at(ctx->window, rect.position.x + box_size + 6, rect.position.y + (rect.size.height - 10) / 2,
            label, ctx->theme->text, 1);

    if (inside && ctx->prev_mouse.left && !ctx->mouse.left) {
        state->checked = !state->checked;
        return true;
    }
    return false;
}

bool uilib_radiobutton(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *label, int value, uilib_radiobutton_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    int box_size = 14;
    rect_t box = { { rect.position.x, rect.position.y + (rect.size.height - box_size) / 2 },
                   { box_size, box_size } };
    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, box.position.x, box.position.y, box.size.width, box.size.height);

    draw_circle(ctx->window, (circle_t){ { box.position.x + box_size/2, box.position.y + box_size/2 }, box_size/2 - 1 }, ctx->theme->border);
    if (state->selected == value)
        fill_circle(ctx->window, (circle_t){ { box.position.x + box_size/2, box.position.y + box_size/2 }, 4 }, ctx->theme->text);
    if (label && label[0])
        draw_text_at(ctx->window, rect.position.x + box_size + 6, rect.position.y + (rect.size.height - 10) / 2,
            label, ctx->theme->text, 1);

    if (inside && ctx->prev_mouse.left && !ctx->mouse.left) {
        state->selected = value;
        return true;
    }
    return false;
}

/* -----------------------------------------------------------------------------
 * TextBox
 * ----------------------------------------------------------------------------- */
void uilib_textbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_textbox_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);

    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, rect.position.x, rect.position.y, rect.size.width, rect.size.height);
    if (inside && ctx->prev_mouse.left && !ctx->mouse.left)
        ctx->active_id = 1;  /* focus */

    /* Simple key input */
    for (int k = KEY_A; k <= KEY_Z; k++) {
        if (is_key_pressed((key_t)k)) {
            char c = (char)('a' + (k - KEY_A));
            if (ctx->active_id == 1 && state->len < UILIB_TEXTBOX_MAX - 1) {
                state->buf[state->len++] = c;
                state->buf[state->len] = '\0';
                state->caret = state->len;
            }
        }
    }
    for (int k = KEY_0; k <= KEY_9; k++) {
        if (is_key_pressed((key_t)k)) {
            char c = (char)('0' + (k - KEY_0));
            if (ctx->active_id == 1 && state->len < UILIB_TEXTBOX_MAX - 1) {
                state->buf[state->len++] = c;
                state->buf[state->len] = '\0';
                state->caret = state->len;
            }
        }
    }
    /* Backspace: Prism has no KEY_BACKSPACE; use Ctrl+H or leave as-is */
    if (ctx->active_id == 1 && is_key_down(KEY_CTRL) && is_key_pressed(KEY_H)) {
        if (state->len > 0 && state->caret > 0) {
            memmove(&state->buf[state->caret - 1], &state->buf[state->caret], (size_t)(state->len - state->caret + 1));
            state->len--;
            state->caret--;
        }
    }

    draw_text_at(ctx->window, rect.position.x + 4, rect.position.y + (rect.size.height - 10) / 2,
        state->buf[0] ? state->buf : "", ctx->theme->text, 1);
}

/* -----------------------------------------------------------------------------
 * ListBox
 * ----------------------------------------------------------------------------- */
bool uilib_listbox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_listbox_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    int line_h = 18;
    int visible = rect.size.height / line_h;
    if (visible <= 0) visible = 1;
    if (state->scroll_offset > state->count - visible) state->scroll_offset = state->count > visible ? state->count - visible : 0;
    if (state->scroll_offset < 0) state->scroll_offset = 0;

    bool changed = false;
    for (int i = 0; i < visible && (state->scroll_offset + i) < state->count; i++) {
        int idx = state->scroll_offset + i;
        rect_t item_rect = { { rect.position.x + 2, rect.position.y + 2 + i * line_h },
                            { rect.size.width - 4, line_h - 2 } };
        int inside = in_rect(ctx->mouse.x, ctx->mouse.y, item_rect.position.x, item_rect.position.y,
            item_rect.size.width, item_rect.size.height);
        if (inside) {
            fill_rect(ctx->window, item_rect, ctx->theme->button_hover);
            if (ctx->prev_mouse.left && !ctx->mouse.left) {
                state->selected = idx;
                changed = true;
            }
        } else if (state->selected == idx) {
            fill_rect(ctx->window, item_rect, ctx->theme->selected);
        }
        if (state->items[idx])
            draw_text_at(ctx->window, item_rect.position.x + 4, item_rect.position.y + 2,
                state->items[idx], ctx->theme->text, 1);
    }
    return changed;
}

/* -----------------------------------------------------------------------------
 * ComboBox
 * ----------------------------------------------------------------------------- */
bool uilib_combobox(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_combobox_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    int btn_w = 20;
    rect_t btn_rect = { { rect.position.x + rect.size.width - btn_w, rect.position.y }, { btn_w, rect.size.height } };
    int btn_inside = in_rect(ctx->mouse.x, ctx->mouse.y, btn_rect.position.x, btn_rect.position.y, btn_w, rect.size.height);
    int main_inside = in_rect(ctx->mouse.x, ctx->mouse.y, rect.position.x, rect.position.y, rect.size.width, rect.size.height);

    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    if (state->count > 0 && state->selected >= 0 && state->selected < state->count && state->items[state->selected])
        draw_text_at(ctx->window, rect.position.x + 4, rect.position.y + (rect.size.height - 10) / 2,
            state->items[state->selected], ctx->theme->text, 1);
    fill_rect(ctx->window, btn_rect, btn_inside ? ctx->theme->button_hover : ctx->theme->button);
    draw_text_at(ctx->window, btn_rect.position.x + 4, btn_rect.position.y + 4, "v", ctx->theme->text, 1);

    bool changed = false;
    if (btn_inside && ctx->prev_mouse.left && !ctx->mouse.left)
        state->dropped = !state->dropped;

    if (state->dropped) {
        int drop_h = state->count * 20;
        rect_t drop = { { rect.position.x, rect.position.y + rect.size.height }, { rect.size.width, drop_h } };
        fill_rect(ctx->window, drop, ctx->theme->panel);
        draw_rect(ctx->window, drop, ctx->theme->border);
        for (int i = 0; i < state->count; i++) {
            rect_t item = { { drop.position.x + 2, drop.position.y + 2 + i * 20 }, { drop.size.width - 4, 18 } };
            int inside = in_rect(ctx->mouse.x, ctx->mouse.y, item.position.x, item.position.y, item.size.width, item.size.height);
            if (inside) {
                fill_rect(ctx->window, item, ctx->theme->button_hover);
                if (ctx->prev_mouse.left && !ctx->mouse.left) {
                    state->selected = i;
                    state->dropped = false;
                    changed = true;
                }
            } else if (state->selected == i) {
                fill_rect(ctx->window, item, ctx->theme->selected);
            }
            if (state->items[i])
                draw_text_at(ctx->window, item.position.x + 4, item.position.y + 2, state->items[i], ctx->theme->text, 1);
        }
        if (!main_inside && !btn_inside && ctx->prev_mouse.left && !ctx->mouse.left)
            state->dropped = false;
    }
    return changed;
}

/* -----------------------------------------------------------------------------
 * ScrollBar
 * ----------------------------------------------------------------------------- */
bool uilib_scrollbar(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_scrollbar_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    int range = state->max_val - state->min_val + 1;
    if (range <= 0) range = 1;
    if (state->page_size <= 0 || state->page_size >= range) state->page_size = 1;
    int track = vertical ? rect.size.height : rect.size.width;
    int thumb = track * state->page_size / range;
    if (thumb < 20) thumb = 20;
    int max_thumb = track - thumb;
    int value_range = range - state->page_size;
    if (value_range < 1) value_range = 1;
    int thumb_pos = (state->value - state->min_val) * max_thumb / value_range;

    rect_t track_rect = rect;
    rect_t thumb_rect;
    if (vertical) {
        thumb_rect = uilib_rect(rect.position.x, rect.position.y + thumb_pos, rect.size.width, thumb);
    } else {
        thumb_rect = uilib_rect(rect.position.x + thumb_pos, rect.position.y, thumb, rect.size.height);
    }

    fill_rect(ctx->window, track_rect, ctx->theme->panel);
    int thumb_inside = in_rect(ctx->mouse.x, ctx->mouse.y, thumb_rect.position.x, thumb_rect.position.y,
        thumb_rect.size.width, thumb_rect.size.height);
    color_t thumb_c = thumb_inside ? ctx->theme->button_hover : ctx->theme->button;
    fill_rect(ctx->window, thumb_rect, thumb_c);
    draw_rect(ctx->window, rect, ctx->theme->border);

    bool changed = false;
    if (thumb_inside && ctx->mouse.left) {
        if (vertical) {
            int rel = ctx->mouse.y - rect.position.y - thumb / 2;
            state->value = state->min_val + rel * value_range / max_thumb;
        } else {
            int rel = ctx->mouse.x - rect.position.x - thumb / 2;
            state->value = state->min_val + rel * value_range / max_thumb;
        }
        if (state->value < state->min_val) state->value = state->min_val;
        if (state->value > state->max_val) state->value = state->max_val;
        changed = true;
    }
    return changed;
}

/* -----------------------------------------------------------------------------
 * ProgressBar, Slider
 * ----------------------------------------------------------------------------- */
void uilib_progressbar(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_progressbar_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    float v = state->value;
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;
    int fill_w = (int)(rect.size.width * v);
    if (fill_w > 0) {
        rect_t fill = { rect.position, { fill_w, rect.size.height } };
        fill_rect(ctx->window, fill, ctx->theme->selected);
    }
    draw_rect(ctx->window, rect, ctx->theme->border);
}

bool uilib_slider(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_slider_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return false;
    float v = state->value;
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;

    int thumb_size = 16;
    int track = vertical ? rect.size.height - thumb_size : rect.size.width - thumb_size;
    int thumb_pos = (int)(v * track);
    rect_t thumb_rect;
    if (vertical) {
        thumb_rect = uilib_rect(rect.position.x, rect.position.y + thumb_pos, rect.size.width, thumb_size);
    } else {
        thumb_rect = uilib_rect(rect.position.x + thumb_pos, rect.position.y, thumb_size, rect.size.height);
    }

    fill_rect(ctx->window, rect, ctx->theme->panel);
    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, thumb_rect.position.x, thumb_rect.position.y,
        thumb_rect.size.width, thumb_rect.size.height);
    color_t thumb_c = inside ? ctx->theme->button_hover : ctx->theme->button;
    fill_rect(ctx->window, thumb_rect, thumb_c);
    draw_rect(ctx->window, rect, ctx->theme->border);

    bool changed = false;
    int track_inside = in_rect(ctx->mouse.x, ctx->mouse.y, rect.position.x, rect.position.y, rect.size.width, rect.size.height);
    if (track_inside && ctx->mouse.left) {
        if (vertical) {
            int rel = ctx->mouse.y - rect.position.y - thumb_size / 2;
            v = (float)rel / (float)track;
        } else {
            int rel = ctx->mouse.x - rect.position.x - thumb_size / 2;
            v = (float)rel / (float)track;
        }
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        state->value = v;
        changed = true;
    }
    return changed;
}

/* -----------------------------------------------------------------------------
 * PictureBox
 * ----------------------------------------------------------------------------- */
void uilib_picture_box(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_image_t *img) {
    if (!ctx || !ctx->window) return;
    if (img && img->pixels) {
        draw_image(ctx->window, *img, rect.position);
    } else {
        fill_rect(ctx->window, rect, ctx->theme ? ctx->theme->panel : (color_t){ 48, 48, 56, 255 });
        draw_rect(ctx->window, rect, ctx->theme ? ctx->theme->border : (color_t){ 90, 90, 100, 255 });
    }
}

/* -----------------------------------------------------------------------------
 * TabControl
 * ----------------------------------------------------------------------------- */
void uilib_tab_control_begin(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_tab_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return;
    int tab_h = 24;
    rect_t tab_bar = { rect.position, { rect.size.width, tab_h } };
    fill_rect(ctx->window, tab_bar, ctx->theme->panel);
    int x = rect.position.x + 4;
    for (int i = 0; i < state->count; i++) {
        int w = 60;
        rect_t tab = { { x, rect.position.y + 2 }, { w, tab_h - 4 } };
        int inside = in_rect(ctx->mouse.x, ctx->mouse.y, tab.position.x, tab.position.y, tab.size.width, tab.size.height);
        if (inside) fill_rect(ctx->window, tab, ctx->theme->button_hover);
        if (state->selected == i) fill_rect(ctx->window, tab, ctx->theme->selected);
        if (state->tabs[i])
            draw_text_at(ctx->window, tab.position.x + 4, tab.position.y + 4, state->tabs[i], ctx->theme->text, 1);
        if (inside && ctx->prev_mouse.left && !ctx->mouse.left) state->selected = i;
        x += w + 2;
    }
    draw_rect(ctx->window, rect, ctx->theme->border);
    rect_t content = { { rect.position.x + 2, rect.position.y + tab_h + 2 },
                      { rect.size.width - 4, rect.size.height - tab_h - 4 } };
    fill_rect(ctx->window, content, ctx->theme->background);
}

void uilib_tab_control_end(uilib_ctx_t *ctx) {
    (void)ctx;
}

/* -----------------------------------------------------------------------------
 * Splitter
 * ----------------------------------------------------------------------------- */
void uilib_splitter(uilib_ctx_t *ctx, uilib_rectangle_t rect, bool vertical, uilib_splitter_state_t *state) {
    if (!ctx || !ctx->window || !ctx->theme || !state) return;
    int grip = 6;
    rect_t grip_rect;
    if (vertical) {
        int y = rect.position.y + state->pos - grip / 2;
        grip_rect = uilib_rect(rect.position.x, y, rect.size.width, grip);
    } else {
        int x = rect.position.x + state->pos - grip / 2;
        grip_rect = uilib_rect(x, rect.position.y, grip, rect.size.height);
    }
    int inside = in_rect(ctx->mouse.x, ctx->mouse.y, grip_rect.position.x, grip_rect.position.y,
        grip_rect.size.width, grip_rect.size.height);
    fill_rect(ctx->window, grip_rect, inside ? ctx->theme->button_hover : ctx->theme->border);
    if (inside && ctx->mouse.left) state->dragging = true;
    if (!ctx->mouse.left) state->dragging = false;
    if (state->dragging) {
        if (vertical)
            state->pos = ctx->mouse.y - rect.position.y;
        else
            state->pos = ctx->mouse.x - rect.position.x;
        if (state->pos < 0) state->pos = 0;
    }
}

/* -----------------------------------------------------------------------------
 * Menus
 * ----------------------------------------------------------------------------- */
bool uilib_menu_strip(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_menu_strip_t *strip, int *out_id) {
    if (!ctx || !ctx->window || !ctx->theme || !strip) return false;
    if (out_id) *out_id = -1;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    int x = rect.position.x + 4;
    for (int m = 0; m < strip->count; m++) {
        int w = 60;
        rect_t menu_rect = { { x, rect.position.y }, { w, rect.size.height } };
        int inside = in_rect(ctx->mouse.x, ctx->mouse.y, menu_rect.position.x, menu_rect.position.y, w, rect.size.height);
        if (inside) fill_rect(ctx->window, menu_rect, ctx->theme->button_hover);
        if (strip->menu_labels[m])
            draw_text_at(ctx->window, menu_rect.position.x + 4, menu_rect.position.y + 6,
                strip->menu_labels[m], ctx->theme->text, 1);
        x += w;
    }
    draw_rect(ctx->window, rect, ctx->theme->border);
    return false;
}

bool uilib_context_menu(uilib_ctx_t *ctx, uilib_context_menu_t *menu, int *out_id) {
    if (!ctx || !ctx->window || !ctx->theme || !menu) return false;
    *out_id = -1;
    if (!menu->open) return false;
    int w = 120, h = menu->count * 22 + 4;
    rect_t r = { menu->position, { w, h } };
    fill_rect(ctx->window, r, ctx->theme->panel);
    draw_rect(ctx->window, r, ctx->theme->border);
    for (int i = 0; i < menu->count; i++) {
        rect_t item = { { r.position.x + 2, r.position.y + 2 + i * 22 }, { w - 4, 20 } };
        int inside = in_rect(ctx->mouse.x, ctx->mouse.y, item.position.x, item.position.y, item.size.width, item.size.height);
        if (inside && menu->items[i].enabled) {
            fill_rect(ctx->window, item, ctx->theme->button_hover);
            if (ctx->prev_mouse.left && !ctx->mouse.left) {
                *out_id = menu->items[i].id;
                menu->open = false;
                return true;
            }
        }
        if (menu->items[i].label)
            draw_text_at(ctx->window, item.position.x + 4, item.position.y + 4,
                menu->items[i].label, menu->items[i].enabled ? ctx->theme->text : ctx->theme->text_dim, 1);
    }
    return false;
}

void uilib_status_strip(uilib_ctx_t *ctx, uilib_rectangle_t rect, const char *text) {
    if (!ctx || !ctx->window || !ctx->theme) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    if (text) draw_text_at(ctx->window, rect.position.x + 8, rect.position.y + (rect.size.height - 10) / 2,
        text, ctx->theme->text_dim, 1);
}

/* -----------------------------------------------------------------------------
 * PropertyGrid
 * ----------------------------------------------------------------------------- */
void uilib_property_grid(uilib_ctx_t *ctx, uilib_rectangle_t rect, uilib_property_grid_t *grid) {
    if (!ctx || !ctx->window || !ctx->theme || !grid) return;
    fill_rect(ctx->window, rect, ctx->theme->panel);
    draw_rect(ctx->window, rect, ctx->theme->border);
    int row_h = 20;
    for (int i = 0; i < grid->count; i++) {
        int y = rect.position.y + 4 + i * row_h;
        if (grid->props[i].name)
            draw_text_at(ctx->window, rect.position.x + 8, y, grid->props[i].name, ctx->theme->text_dim, 1);
        char buf[32];
        switch (grid->props[i].type) {
            case UILIB_PROP_INT: snprintf(buf, sizeof(buf), "%d", grid->props[i].value.i); break;
            case UILIB_PROP_FLOAT: snprintf(buf, sizeof(buf), "%.2f", grid->props[i].value.f); break;
            case UILIB_PROP_BOOL: snprintf(buf, sizeof(buf), "%s", grid->props[i].value.b ? "true" : "false"); break;
            case UILIB_PROP_STRING: snprintf(buf, sizeof(buf), "%.31s", grid->props[i].value.s); break;
        }
        draw_text_at(ctx->window, rect.position.x + rect.size.width / 2, y, buf, ctx->theme->text, 1);
    }
}

/* -----------------------------------------------------------------------------
 * Dialogs
 * ----------------------------------------------------------------------------- */
#if defined(PRISM_PLATFORM_WINDOWS)
uilib_dialog_result_t uilib_message_box(const char *title, const char *message, uilib_message_buttons_t buttons) {
    UINT flags = 0;
    switch (buttons) {
        case UILIB_DIALOG_OK: flags = MB_OK; break;
        case UILIB_DIALOG_OK_CANCEL: flags = MB_OKCANCEL; break;
        case UILIB_DIALOG_YES_NO: flags = MB_YESNO; break;
    }
    int r = MessageBoxA(NULL, message ? message : "", title ? title : "", flags);
    switch (r) {
        case IDOK: return UILIB_RESULT_OK;
        case IDCANCEL: return UILIB_RESULT_CANCEL;
        case IDYES: return UILIB_RESULT_YES;
        case IDNO: return UILIB_RESULT_NO;
        default: return UILIB_RESULT_NONE;
    }
}

bool uilib_open_file_dialog(uilib_open_file_dialog_t *d) {
    if (!d) return false;
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    d->filename[0] = '\0';
    ofn.lpstrFile = d->filename;
    ofn.nMaxFile = UILIB_FILENAME_MAX;
    ofn.lpstrFilter = d->filter ? d->filter : "All\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return GetOpenFileNameA(&ofn) != 0;
}

bool uilib_save_file_dialog(uilib_save_file_dialog_t *d) {
    if (!d) return false;
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    d->filename[0] = '\0';
    ofn.lpstrFile = d->filename;
    ofn.nMaxFile = UILIB_FILENAME_MAX;
    ofn.lpstrFilter = d->filter ? d->filter : "All\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    return GetSaveFileNameA(&ofn) != 0;
}

#if defined(PRISM_PLATFORM_WINDOWS)
#include <shlobj.h>
bool uilib_folder_browser_dialog(uilib_folder_dialog_t *d) {
    if (!d) return false;
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = "Select folder";
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (!pidl) return false;
    if (SHGetPathFromIDListA(pidl, d->path))
        return true;
    return false;
}
#else
bool uilib_folder_browser_dialog(uilib_folder_dialog_t *d) {
    (void)d;
    return false;
}
#endif

bool uilib_color_dialog(uilib_color_dialog_t *d) {
    if (!d) return false;
    static CHOOSECOLORA cc;
    static int init;
    if (!init) { memset(&cc, 0, sizeof(cc)); cc.lStructSize = sizeof(cc); init = 1; }
    static COLORREF cust[16] = { 0 };
    cc.lpCustColors = cust;
    cc.rgbResult = RGB(d->color.r, d->color.g, d->color.b);
    if (ChooseColorA(&cc)) {
        d->color.r = GetRValue(cc.rgbResult);
        d->color.g = GetGValue(cc.rgbResult);
        d->color.b = GetBValue(cc.rgbResult);
        d->color.a = 255;
        return true;
    }
    return false;
}

bool uilib_font_dialog(uilib_font_dialog_t *d) {
    if (!d) return false;
    CHOOSEFONTA cf;
    memset(&cf, 0, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.Flags = CF_SCREENFONTS;
    static LOGFONTA lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = d->font.size * 12;
    cf.lpLogFont = &lf;
    if (ChooseFontA(&cf)) {
        d->font.size = (int)(lf.lfHeight / 12);
        if (d->font.size < 1) d->font.size = 1;
        return true;
    }
    return false;
}

#else
uilib_dialog_result_t uilib_message_box(const char *title, const char *message, uilib_message_buttons_t buttons) {
    (void)title;
    (void)message;
    (void)buttons;
    return UILIB_RESULT_NONE;
}

bool uilib_open_file_dialog(uilib_open_file_dialog_t *d) {
    (void)d;
    return false;
}

bool uilib_save_file_dialog(uilib_save_file_dialog_t *d) {
    (void)d;
    return false;
}

bool uilib_folder_browser_dialog(uilib_folder_dialog_t *d) {
    (void)d;
    return false;
}

bool uilib_color_dialog(uilib_color_dialog_t *d) {
    (void)d;
    return false;
}

bool uilib_font_dialog(uilib_font_dialog_t *d) {
    (void)d;
    return false;
}
#endif
