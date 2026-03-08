/*
 * UILib demo – comprehensive showcase of widgets.
 * Build from repo root: make uilib_demo && ./uilib_demo
 */
#include "../src/prism.h"
#include "uilib.h"
#include <stdlib.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    static void sleep_ms(int ms) { Sleep(ms); }
#else
    #include <unistd.h>
    static void sleep_ms(int ms) { usleep(ms * 1000); }
#endif

int main(void) {
    prism_size_t size = { 640, 480 };
    position_t pos = { 80, 80 };
    window_t *window = create_window(size, pos, 0);
    if (!window) return 1;

    uilib_ctx_t ui = { 0 };
    int counter = 0;

    uilib_checkbox_state_t cb_state = { false };
    uilib_radiobutton_state_t rb_state = { 0 };
    uilib_textbox_state_t tb_state = { { 0 }, 0, 0, 0 };
    uilib_slider_state_t slider_state = { 0.5f };
    uilib_progressbar_state_t prog_state = { 0.f };
    uilib_tab_state_t tab_state = { { "One", "Two", "Three", NULL }, 3, 0 };
    uilib_listbox_state_t lb_state = { 0 };
    lb_state.items[0] = "Apple";
    lb_state.items[1] = "Banana";
    lb_state.items[2] = "Cherry";
    lb_state.count = 3;
    lb_state.selected = 0;

    uilib_combobox_state_t combo_state = { 0 };
    combo_state.items[0] = "Red";
    combo_state.items[1] = "Green";
    combo_state.items[2] = "Blue";
    combo_state.count = 3;
    combo_state.selected = 0;
    combo_state.dropped = false;

    int running = 1;
    while (running) {
        clear_window(window, (color_t){ 32, 32, 36, 255 });
        uilib_begin(&ui, window, &UILIB_THEME_DARK);

        uilib_label(&ui, uilib_point(20, 12), "UILib Demo - Full Widget Set", false);

        /* Left column - buttons, checkbox, radio */
        uilib_rectangle_t left = uilib_rect(20, 36, 200, 200);
        uilib_panel(&ui, left);
        uilib_label(&ui, uilib_point(28, 44), "Controls", false);

        uilib_rectangle_t r;
        uilib_stack_begin(&ui, uilib_rect(28, 60, 184, 170), UILIB_ORIENT_VERT, 4);
        if (uilib_stack_next(&ui, 28, &r) && uilib_button(&ui, r, "Click me")) counter++;
        if (uilib_stack_next(&ui, 28, &r) && uilib_button(&ui, r, "Reset")) counter = 0;
        if (uilib_stack_next(&ui, 28, &r) && uilib_button(&ui, r, "Quit")) running = 0;
        if (uilib_stack_next(&ui, 22, &r)) uilib_checkbox(&ui, r, "Check me", &cb_state);
        if (uilib_stack_next(&ui, 22, &r)) uilib_radiobutton(&ui, r, "Option A", 0, &rb_state);
        if (uilib_stack_next(&ui, 22, &r)) uilib_radiobutton(&ui, r, "Option B", 1, &rb_state);
        uilib_stack_next(&ui, 22, &r);

        /* Center - textbox, slider, progress */
        uilib_rectangle_t center = uilib_rect(236, 36, 180, 200);
        uilib_panel(&ui, center);
        uilib_label(&ui, uilib_point(244, 44), "Input", false);
        uilib_stack_begin(&ui, uilib_rect(244, 60, 164, 170), UILIB_ORIENT_VERT, 6);
        if (uilib_stack_next(&ui, 24, &r)) uilib_textbox(&ui, r, &tb_state);
        if (uilib_stack_next(&ui, 24, &r)) uilib_slider(&ui, r, false, &slider_state);
        if (uilib_stack_next(&ui, 20, &r)) {
            prog_state.value = slider_state.value;
            uilib_progressbar(&ui, r, &prog_state);
        }
        uilib_stack_next(&ui, 20, &r);

        /* Right - list, combo */
        uilib_rectangle_t right = uilib_rect(432, 36, 192, 200);
        uilib_panel(&ui, right);
        uilib_label(&ui, uilib_point(440, 44), "Selection", false);
        uilib_listbox(&ui, uilib_rect(440, 60, 176, 80), &lb_state);
        uilib_combobox(&ui, uilib_rect(440, 148, 176, 24), &combo_state);
        uilib_label(&ui, uilib_point(440, 178), "ComboBox above", true);

        /* Tab control */
        uilib_tab_control_begin(&ui, uilib_rect(20, 248, 604, 180), &tab_state);
        uilib_tab_control_end(&ui);

        /* Status */
        char buf[128];
        snprintf(buf, sizeof(buf), "Clicks: %d  Checkbox: %s  Radio: %d  Slider: %.2f", 
            counter, cb_state.checked ? "on" : "off", rb_state.selected, (double)slider_state.value);
        uilib_status_strip(&ui, uilib_rect(20, 436, 604, 32), buf);
        uilib_label(&ui, uilib_point(20, 468), "Press Q to exit", true);

        present_window(window);
        if (is_key_down(KEY_Q)) running = 0;
        sleep_ms(16);
    }

    destroy_window(window);
    return 0;
}
