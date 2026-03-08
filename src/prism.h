#include "../include/prism.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#if defined(PRISM_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif defined(PRISM_PLATFORM_LINUX)
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
#endif

static inline int prism_pack_color(color_t c) {
    uint8_t r = (uint8_t)c.r;
    uint8_t g = (uint8_t)c.g;
    uint8_t b = (uint8_t)c.b;
    uint8_t a = (uint8_t)c.a;
    return ((int)a << 24) | ((int)r << 16) | ((int)g << 8) | (int)b;
}

static inline color_t prism_unpack_color(int value) {
    color_t c;
    c.a = (value >> 24) & 0xFF;
    c.r = (value >> 16) & 0xFF;
    c.g = (value >> 8) & 0xFF;
    c.b = value & 0xFF;
    return c;
}

static inline int prism_in_bounds(const window_t *window, int x, int y) {
    return x >= 0 && y >= 0 && x < window->width && y < window->height;
}

static inline int prism_index(const window_t *window, int x, int y) {
    return y * window->width + x;
}

window_t *create_window(prism_size_t size, position_t position, int flags) {
    window_t *window = (window_t *)malloc(sizeof(window_t));
    if (!window) {
        return NULL;
    }

    window->width = size.width;
    window->height = size.height;
    window->x = position.x;
    window->y = position.y;
    window->flags = flags;

    size_t pixel_count = (size_t)(size.width * size.height);
    window->pixels = (int *)calloc(pixel_count, sizeof(int));
    if (!window->pixels) {
        free(window);
        return NULL;
    }

#if defined(PRISM_PLATFORM_WINDOWS)
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PrismWindowClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        "PrismWindowClass",
        "Prism Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        position.x,
        position.y,
        size.width,
        size.height,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hwnd) {
        free(window->pixels);
        free(window);
        return NULL;
    }

    HDC hdc = GetDC(hwnd);
    window->platform_hwnd = hwnd;
    window->platform_hdc = hdc;
#elif defined(PRISM_PLATFORM_LINUX)
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        free(window->pixels);
        free(window);
        return NULL;
    }

    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    Window win = XCreateSimpleWindow(
        display,
        root,
        position.x,
        position.y,
        (unsigned int)size.width,
        (unsigned int)size.height,
        0,
        BlackPixel(display, screen),
        BlackPixel(display, screen));

    XStoreName(display, win, "Prism Window");

    XSelectInput(display, win,
                 ExposureMask |
                 KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask |
                 StructureNotifyMask);

    XMapWindow(display, win);
    XFlush(display);

    window->platform_display = display;
    window->platform_window = win;
#elif defined(PRISM_PLATFORM_MACOS)
    window->platform_window = NULL;
    window->platform_view = NULL;
#endif

    return window;
}

void destroy_window(window_t *window) {
    if (!window) {
        return;
    }

#if defined(PRISM_PLATFORM_WINDOWS)
    if (window->platform_hwnd && window->platform_hdc) {
        ReleaseDC((HWND)window->platform_hwnd, (HDC)window->platform_hdc);
    }
    if (window->platform_hwnd) {
        DestroyWindow((HWND)window->platform_hwnd);
    }
#elif defined(PRISM_PLATFORM_LINUX)
    if (window->platform_display && window->platform_window) {
        Display *display = (Display *)window->platform_display;
        Window win = (Window)window->platform_window;
        XDestroyWindow(display, win);
        XCloseDisplay(display);
    }
#endif

    free(window->pixels);
    window->pixels = NULL;
    free(window);
}

void clear_window(window_t *window, color_t color) {
    if (!window || !window->pixels) {
        return;
    }
    int packed = prism_pack_color(color);
    size_t count = (size_t)(window->width * window->height);
    for (size_t i = 0; i < count; ++i) {
        window->pixels[i] = packed;
    }
}

void present_window(window_t *window) {
    if (!window) {
        return;
    }

#if defined(PRISM_PLATFORM_WINDOWS)
    HWND hwnd = (HWND)window->platform_hwnd;
    HDC hdc = (HDC)window->platform_hdc;
    if (!hwnd || !hdc || !window->pixels) {
        return;
    }

    MSG msg;
    while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = window->width;
    bmi.bmiHeader.biHeight = -window->height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(
        hdc,
        0,
        0,
        window->width,
        window->height,
        0,
        0,
        window->width,
        window->height,
        window->pixels,
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY);
#elif defined(PRISM_PLATFORM_LINUX)
    if (!window->platform_display || !window->platform_window || !window->pixels) {
        return;
    }

    Display *display = (Display *)window->platform_display;
    Window win = (Window)window->platform_window;

    while (XPending(display) > 0) {
        XEvent ev;
        XNextEvent(display, &ev);
        /* Events are currently ignored; applications control exit conditions. */
    }

    int screen = DefaultScreen(display);
    int depth = DefaultDepth(display, screen);
    if (depth != 24 && depth != 32) {
        depth = 24;
    }

    XImage *img = XCreateImage(
        display,
        DefaultVisual(display, screen),
        (unsigned int)depth,
        ZPixmap,
        0,
        NULL,
        (unsigned int)window->width,
        (unsigned int)window->height,
        32,
        0);

    if (!img) {
        return;
    }

    img->data = (char *)malloc((size_t)img->bytes_per_line * (size_t)img->height);
    if (!img->data) {
        img->data = NULL;
        XDestroyImage(img);
        return;
    }

    int bpp = img->bits_per_pixel / 8;
    for (int y = 0; y < window->height; ++y) {
        for (int x = 0; x < window->width; ++x) {
            int idx = prism_index(window, x, y);
            color_t c = prism_unpack_color(window->pixels[idx]);

            unsigned char *dst = (unsigned char *)img->data +
                                 (size_t)y * (size_t)img->bytes_per_line +
                                 (size_t)x * (size_t)bpp;

            dst[0] = (unsigned char)c.b;
            if (bpp > 1) dst[1] = (unsigned char)c.g;
            if (bpp > 2) dst[2] = (unsigned char)c.r;
            if (bpp > 3) dst[3] = 0xFF;
        }
    }

    GC gc = XCreateGC(display, win, 0, NULL);
    XPutImage(
        display,
        win,
        gc,
        img,
        0,
        0,
        0,
        0,
        (unsigned int)window->width,
        (unsigned int)window->height);

    XFreeGC(display, gc);
    XDestroyImage(img);
    XFlush(display);
#elif defined(PRISM_PLATFORM_MACOS)
    (void)window;
#else
    (void)window;
#endif
}

#if defined(PRISM_PLATFORM_WINDOWS)
bool is_key_pressed(key_t key) {
    int vk = 0;
    switch (key) {
        case KEY_A: vk = 'A'; break;
        case KEY_B: vk = 'B'; break;
        case KEY_C: vk = 'C'; break;
        case KEY_D: vk = 'D'; break;
        case KEY_E: vk = 'E'; break;
        case KEY_F: vk = 'F'; break;
        case KEY_G: vk = 'G'; break;
        case KEY_H: vk = 'H'; break;
        case KEY_I: vk = 'I'; break;
        case KEY_J: vk = 'J'; break;
        case KEY_K: vk = 'K'; break;
        case KEY_L: vk = 'L'; break;
        case KEY_M: vk = 'M'; break;
        case KEY_N: vk = 'N'; break;
        case KEY_O: vk = 'O'; break;
        case KEY_P: vk = 'P'; break;
        case KEY_Q: vk = 'Q'; break;
        case KEY_R: vk = 'R'; break;
        case KEY_S: vk = 'S'; break;
        case KEY_T: vk = 'T'; break;
        case KEY_U: vk = 'U'; break;
        case KEY_V: vk = 'V'; break;
        case KEY_W: vk = 'W'; break;
        case KEY_X: vk = 'X'; break;
        case KEY_Y: vk = 'Y'; break;
        case KEY_Z: vk = 'Z'; break;
        case KEY_0: vk = '0'; break;
        case KEY_1: vk = '1'; break;
        case KEY_2: vk = '2'; break;
        case KEY_3: vk = '3'; break;
        case KEY_4: vk = '4'; break;
        case KEY_5: vk = '5'; break;
        case KEY_6: vk = '6'; break;
        case KEY_7: vk = '7'; break;
        case KEY_8: vk = '8'; break;
        case KEY_9: vk = '9'; break;
        case KEY_UP: vk = VK_UP; break;
        case KEY_DOWN: vk = VK_DOWN; break;
        case KEY_LEFT: vk = VK_LEFT; break;
        case KEY_RIGHT: vk = VK_RIGHT; break;
        case KEY_CTRL: vk = VK_CONTROL; break;
        case KEY_ALT: vk = VK_MENU; break;
        case KEY_SHIFT: vk = VK_SHIFT; break;
        default: return false;
    }

    SHORT state = GetAsyncKeyState(vk);
    return (state & 0x0001) != 0;
}

bool is_key_down(key_t key) {
    int vk = 0;
    switch (key) {
        case KEY_A: vk = 'A'; break;
        case KEY_B: vk = 'B'; break;
        case KEY_C: vk = 'C'; break;
        case KEY_D: vk = 'D'; break;
        case KEY_E: vk = 'E'; break;
        case KEY_F: vk = 'F'; break;
        case KEY_G: vk = 'G'; break;
        case KEY_H: vk = 'H'; break;
        case KEY_I: vk = 'I'; break;
        case KEY_J: vk = 'J'; break;
        case KEY_K: vk = 'K'; break;
        case KEY_L: vk = 'L'; break;
        case KEY_M: vk = 'M'; break;
        case KEY_N: vk = 'N'; break;
        case KEY_O: vk = 'O'; break;
        case KEY_P: vk = 'P'; break;
        case KEY_Q: vk = 'Q'; break;
        case KEY_R: vk = 'R'; break;
        case KEY_S: vk = 'S'; break;
        case KEY_T: vk = 'T'; break;
        case KEY_U: vk = 'U'; break;
        case KEY_V: vk = 'V'; break;
        case KEY_W: vk = 'W'; break;
        case KEY_X: vk = 'X'; break;
        case KEY_Y: vk = 'Y'; break;
        case KEY_Z: vk = 'Z'; break;
        case KEY_0: vk = '0'; break;
        case KEY_1: vk = '1'; break;
        case KEY_2: vk = '2'; break;
        case KEY_3: vk = '3'; break;
        case KEY_4: vk = '4'; break;
        case KEY_5: vk = '5'; break;
        case KEY_6: vk = '6'; break;
        case KEY_7: vk = '7'; break;
        case KEY_8: vk = '8'; break;
        case KEY_9: vk = '9'; break;
        case KEY_UP: vk = VK_UP; break;
        case KEY_DOWN: vk = VK_DOWN; break;
        case KEY_LEFT: vk = VK_LEFT; break;
        case KEY_RIGHT: vk = VK_RIGHT; break;
        case KEY_CTRL: vk = VK_CONTROL; break;
        case KEY_ALT: vk = VK_MENU; break;
        case KEY_SHIFT: vk = VK_SHIFT; break;
        default: return false;
    }

    SHORT state = GetAsyncKeyState(vk);
    return (state & 0x8000) != 0;
}

mouse_t get_mouse(window_t *window) {
    mouse_t m = {0, 0, false, false, false};
    if (!window || !window->platform_hwnd) {
        return m;
    }

    POINT p;
    if (GetCursorPos(&p)) {
        ScreenToClient((HWND)window->platform_hwnd, &p);
        m.x = p.x;
        m.y = p.y;
    }

    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) m.left = true;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) m.middle = true;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) m.right = true;

    return m;
}
#elif defined(PRISM_PLATFORM_LINUX) || defined(PRISM_PLATFORM_MACOS) || defined(PRISM_PLATFORM_UNKNOWN)
bool is_key_pressed(key_t key) {
    (void)key;
    return false;
}

bool is_key_down(key_t key) {
    (void)key;
    return false;
}

mouse_t get_mouse(window_t *window) {
    (void)window;
    mouse_t m = {0, 0, false, false, false};
    return m;
}
#endif

void draw_pixel(window_t *window, position_t position, color_t color) {
    if (!window || !window->pixels) {
        return;
    }
    if (!prism_in_bounds(window, position.x, position.y)) {
        return;
    }
    int idx = prism_index(window, position.x, position.y);
    window->pixels[idx] = prism_pack_color(color);
}

color_t get_pixel(window_t *window, position_t position) {
    if (!window || !window->pixels) {
        return COLOR_BLACK;
    }
    if (!prism_in_bounds(window, position.x, position.y)) {
        return COLOR_BLACK;
    }
    int idx = prism_index(window, position.x, position.y);
    return prism_unpack_color(window->pixels[idx]);
}

void draw_line(window_t *window, position_t p0, position_t p1, color_t color) {
    if (!window || !window->pixels) {
        return;
    }

    int x0 = p0.x;
    int y0 = p0.y;
    int x1 = p1.x;
    int y1 = p1.y;

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (1) {
        position_t p = {x0, y0};
        draw_pixel(window, p, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_rect(window_t *window, rect_t rect, color_t color) {
    position_t top_left = rect.position;
    position_t top_right = {rect.position.x + rect.size.width - 1, rect.position.y};
    position_t bottom_left = {rect.position.x, rect.position.y + rect.size.height - 1};
    position_t bottom_right = {rect.position.x + rect.size.width - 1, rect.position.y + rect.size.height - 1};

    draw_line(window, top_left, top_right, color);
    draw_line(window, top_left, bottom_left, color);
    draw_line(window, bottom_left, bottom_right, color);
    draw_line(window, top_right, bottom_right, color);
}

void draw_circle(window_t *window, circle_t circle, color_t color) {
    if (!window || !window->pixels) {
        return;
    }

    int x0 = circle.position.x;
    int y0 = circle.position.y;
    int radius = circle.radius;

    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        position_t pts[8] = {
            {x0 + x, y0 + y},
            {x0 + y, y0 + x},
            {x0 - y, y0 + x},
            {x0 - x, y0 + y},
            {x0 - x, y0 - y},
            {x0 - y, y0 - x},
            {x0 + y, y0 - x},
            {x0 + x, y0 - y},
        };
        for (int i = 0; i < 8; ++i) {
            draw_pixel(window, pts[i], color);
        }

        y += 1;
        if (err <= 0) {
            err += 2 * y + 1;
        } else {
            x -= 1;
            err -= 2 * x + 1;
        }
    }
}

void draw_ellipse(window_t *window, ellipse_t ellipse, color_t color) {
    if (!window || !window->pixels) {
        return;
    }

    int xc = ellipse.position.x;
    int yc = ellipse.position.y;
    int rx = ellipse.width;
    int ry = ellipse.height;

    long rx2 = (long)rx * rx;
    long ry2 = (long)ry * ry;
    long x = 0;
    long y = ry;
    long px = 0;
    long py = 2 * rx2 * y;

    long p = (long)(ry2 - (rx2 * ry) + (0.25 * rx2));
    while (px < py) {
        position_t pts[4] = {
            {xc + (int)x, yc + (int)y},
            {xc - (int)x, yc + (int)y},
            {xc + (int)x, yc - (int)y},
            {xc - (int)x, yc - (int)y},
        };
        for (int i = 0; i < 4; ++i) {
            draw_pixel(window, pts[i], color);
        }

        x++;
        px += 2 * ry2;
        if (p < 0) {
            p += ry2 + px;
        } else {
            y--;
            py -= 2 * rx2;
            p += ry2 + px - py;
        }
    }

    p = (long)(ry2 * (x + 0.5) * (x + 0.5) + rx2 * (y - 1) * (y - 1) - rx2 * ry2);
    while (y >= 0) {
        position_t pts[4] = {
            {xc + (int)x, yc + (int)y},
            {xc - (int)x, yc + (int)y},
            {xc + (int)x, yc - (int)y},
            {xc - (int)x, yc - (int)y},
        };
        for (int i = 0; i < 4; ++i) {
            draw_pixel(window, pts[i], color);
        }

        y--;
        py -= 2 * rx2;
        if (p > 0) {
            p += rx2 - py;
        } else {
            x++;
            px += 2 * ry2;
            p += rx2 - py + px;
        }
    }
}

void draw_polygon(window_t *window, polygon_t polygon, color_t color) {
    if (!polygon.points || polygon.count < 2) {
        return;
    }
    for (int i = 0; i < polygon.count - 1; ++i) {
        position_t p0 = polygon.points[i];
        position_t p1 = polygon.points[i + 1];
        draw_line(window, p0, p1, color);
    }
    if (polygon.count > 2) {
        draw_line(window, polygon.points[polygon.count - 1], polygon.points[0], color);
    }
}

void draw_shape(window_t *window, shape_t shape) {
    if (!shape.points || shape.count < 4) {
        return;
    }
    for (int i = 0; i < shape.count - 2; i += 2) {
        position_t p0 = {shape.points[i], shape.points[i + 1]};
        position_t p1 = {shape.points[i + 2], shape.points[i + 3]};
        draw_line(window, p0, p1, COLOR_WHITE);
    }
}

void draw_text(window_t *window, text_t text, color_t color, position_t position, int font_size) {
    (void)window;
    (void)text;
    (void)color;
    (void)position;
    (void)font_size;
    /* Text rendering requires font handling and is left as a no-op. */
}

void draw_image(window_t *window, image_t image, position_t position) {
    if (!window || !window->pixels || !image.pixels) {
        return;
    }
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            int dst_x = position.x + x;
            int dst_y = position.y + y;
            if (!prism_in_bounds(window, dst_x, dst_y)) {
                continue;
            }
            int src_idx = y * image.width + x;
            int dst_idx = prism_index(window, dst_x, dst_y);
            window->pixels[dst_idx] = image.pixels[src_idx];
        }
    }
}

void free_image(image_t *image) {
    if (!image) {
        return;
    }
    free(image->pixels);
    image->pixels = NULL;
    image->width = 0;
    image->height = 0;
}

void free_shape(shape_t *shape) {
    if (!shape) {
        return;
    }
    free(shape->points);
    shape->points = NULL;
    shape->count = 0;
}

void free_text(text_t *text) {
    (void)text;
    /* text->text is assumed to be managed by the caller. */
}

void free_polygon(polygon_t *polygon) {
    if (!polygon) {
        return;
    }
    free(polygon->points);
    polygon->points = NULL;
    polygon->count = 0;
}

void free_ellipse(ellipse_t *ellipse) {
    (void)ellipse;
}

void free_circle(circle_t *circle) {
    (void)circle;
}

void free_rect(rect_t *rect) {
    (void)rect;
}

void free_window(window_t *window) {
    destroy_window(window);
}

void translate(position_t *position, int x, int y) {
    if (!position) {
        return;
    }
    position->x += x;
    position->y += y;
}

void scale(position_t *position, int x, int y) {
    if (!position) {
        return;
    }
    position->x *= x;
    position->y *= y;
}

void rotate(position_t *position, int angle) {
    if (!position) {
        return;
    }
    double radians = (double)angle * (3.14159265358979323846 / 180.0);
    double cos_a = cos(radians);
    double sin_a = sin(radians);
    int x = position->x;
    int y = position->y;
    position->x = (int)round(x * cos_a - y * sin_a);
    position->y = (int)round(x * sin_a + y * cos_a);
}
