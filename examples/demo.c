#include "../src/prism.h"

#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    static void prism_sleep_ms(int ms) { Sleep(ms); }
#else
    #include <unistd.h>
    static void prism_sleep_ms(int ms) { usleep(ms * 1000); }
#endif

int main(void) {
    prism_size_t size = {800, 600};
    position_t pos = {100, 100};

    window_t *window = create_window(size, pos, 0);
    if (!window) {
        return 1;
    }

    image_t logo = load_bmp("image.bmp");

    int running = 1;
    int frame = 0;

    while (running) {
        clear_window(window, COLOR_BLACK);

        position_t center = {window->width / 2, window->height / 2};
        circle_t circle = {center, 100};
        draw_circle(window, circle, COLOR_WHITE);

        rect_t rect;
        rect.position.x = 60;
        rect.position.y = 60;
        rect.size.width = 160;
        rect.size.height = 120;
        draw_rect(window, rect, COLOR_WHITE);

        rect_t rect_fill = rect;
        rect_fill.position.x += 10;
        rect_fill.position.y += 10;
        rect_fill.size.width -= 20;
        rect_fill.size.height -= 20;
        fill_rect(window, rect_fill, (color_t){40, 120, 255, 255});

        ellipse_t ellipse;
        ellipse.position.x = 620;
        ellipse.position.y = 420;
        ellipse.width = 120;
        ellipse.height = 60;
        draw_ellipse(window, ellipse, COLOR_WHITE);

        ellipse_t ellipse_fill = ellipse;
        ellipse_fill.width -= 20;
        ellipse_fill.height -= 10;
        fill_ellipse(window, ellipse_fill, (color_t){255, 120, 40, 255});

        polygon_t poly;
        position_t points[5] = {
            {400, 120},
            {460, 200},
            {430, 260},
            {370, 260},
            {340, 200}
        };
        poly.points = points;
        poly.count = 5;
        draw_polygon(window, poly, COLOR_WHITE);
        fill_polygon(window, poly, (color_t){80, 200, 120, 255});

        circle_t circle_fill = { {200, 400}, 60 };
        fill_circle(window, circle_fill, (color_t){200, 80, 160, 255});

        position_t p0 = {0, 0};
        position_t p1 = {window->width - 1, window->height - 1};
        position_t p2 = {window->width - 1, 0};
        position_t p3 = {0, window->height - 1};
        draw_line(window, p0, p1, COLOR_WHITE);
        draw_line(window, p2, p3, COLOR_WHITE);

        mouse_t mouse = get_mouse(window);
        if (mouse.x >= 0 && mouse.y >= 0 &&
            mouse.x < window->width && mouse.y < window->height) {
            position_t mpos = {mouse.x, mouse.y};
            draw_pixel(window, mpos, COLOR_WHITE);
        }

        position_t anim_pos = {100 + (frame % (window->width - 200)), 520};
        draw_pixel(window, anim_pos, COLOR_WHITE);
        frame++;

        if (logo.pixels) {
            position_t img_pos = {window->width - logo.width - 20, 20};
            draw_image(window, logo, img_pos);
        }

        text_t title = {"PRISM DEMO: FILLED PRIMITIVES, IMAGE, TEXT", 0, 0};
        position_t text_pos = {40, 24};
        draw_text(window, title, COLOR_WHITE, text_pos, 2);

        text_t hint = {"Press Q to quit", 0, 0};
        position_t hint_pos = {40, 60};
        draw_text(window, hint, (color_t){180, 180, 180, 255}, hint_pos, 1);

        present_window(window);

        if (is_key_down(KEY_Q)) {
            running = 0;
        }

        prism_sleep_ms(16);
    }

    free_image(&logo);
    free_window(window);
    return 0;
}

