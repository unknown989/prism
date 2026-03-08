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

    int running = 1;
    int frame = 0;

    while (running) {
        clear_window(window, COLOR_BLACK);

        position_t center = {window->width / 2, window->height / 2};
        circle_t circle = {center, 100};
        draw_circle(window, circle, COLOR_WHITE);

        rect_t rect;
        rect.position.x = 100;
        rect.position.y = 100;
        rect.size.width = 200;
        rect.size.height = 150;
        draw_rect(window, rect, COLOR_WHITE);

        ellipse_t ellipse;
        ellipse.position.x = 600;
        ellipse.position.y = 400;
        ellipse.width = 120;
        ellipse.height = 60;
        draw_ellipse(window, ellipse, COLOR_WHITE);

        polygon_t poly;
        position_t points[5] = {
            {400, 100},
            {450, 150},
            {430, 200},
            {370, 200},
            {350, 150}
        };
        poly.points = points;
        poly.count = 5;
        draw_polygon(window, poly, COLOR_WHITE);

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

        position_t anim_pos = {100 + (frame % (window->width - 200)), 500};
        draw_pixel(window, anim_pos, COLOR_WHITE);
        frame++;

        present_window(window);

        if (is_key_down(KEY_Q)) {
            running = 0;
        }

        prism_sleep_ms(16);
    }

    free_window(window);
    return 0;
}

