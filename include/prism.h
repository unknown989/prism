#ifndef PRISM_H
#define PRISM_H

#include <stdbool.h>

/* Platform detection */
#if defined(_WIN32) || defined(_WIN64)
    #define PRISM_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #define PRISM_PLATFORM_MACOS
#elif defined(__linux__)
    #define PRISM_PLATFORM_LINUX
#else
    #define PRISM_PLATFORM_UNKNOWN
#endif

typedef struct window {
    int width;
    int height;
    int x;
    int y;
    int flags;
    int *pixels;
#if defined(PRISM_PLATFORM_WINDOWS)
    void *platform_hwnd;
    void *platform_hdc;
#elif defined(PRISM_PLATFORM_LINUX)
    void *platform_display;
    unsigned long platform_window;
#elif defined(PRISM_PLATFORM_MACOS)
    void *platform_window;
    void *platform_view;
#endif
} window_t;
typedef struct color {
    int r;
    int g;
    int b;
    int a;
} color_t;
#define COLOR_BLACK (color_t){0, 0, 0, 255}
#define COLOR_WHITE (color_t){255, 255, 255, 255}

typedef enum{
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CTRL,
    KEY_ALT,
    KEY_SHIFT
} key_t;

bool is_key_pressed(key_t key);
bool is_key_down(key_t key);

typedef struct mouse{
    int x;
    int y;
    bool left;
    bool middle;
    bool right;
} mouse_t;
mouse_t get_mouse(window_t *window);

typedef enum{
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
} mouse_button_t;

typedef struct position{
    int x;
    int y;
} position_t;
typedef struct prism_size{
    int width;
    int height;
} prism_size_t;
typedef struct rect{
    position_t position;
    prism_size_t size;
} rect_t;
typedef struct circle{
    position_t position;
    int radius;
} circle_t;
typedef struct ellipse{
    position_t position;
    int width;
    int height;
} ellipse_t;
typedef struct polygon{
    position_t *points;
    int count;
} polygon_t;
typedef struct text{
    const char *text;
    int x;
    int y;
} text_t;
typedef struct image{
    int *pixels;
    int width;
    int height;
} image_t;
typedef struct shape{
    int *points;
    int count;
} shape_t;

window_t *create_window(prism_size_t size, position_t position, int flags);
void destroy_window(window_t *window);
void clear_window(window_t *window, color_t color);
void present_window(window_t *window);

// basic drawing functions
void draw_pixel(window_t *window, position_t position, color_t color);
color_t get_pixel(window_t *window, position_t position);
void draw_line(window_t *window, position_t position1, position_t position2, color_t color);
void draw_rect(window_t *window, rect_t rect, color_t color);
void draw_circle(window_t *window, circle_t circle, color_t color);
void draw_ellipse(window_t *window, ellipse_t ellipse, color_t color);
void draw_polygon(window_t *window, polygon_t polygon, color_t color);
void draw_shape(window_t *window, shape_t shape);

// text rendering functions
void draw_text(window_t *window, text_t text, color_t color, position_t position, int font_size);
// image rendering functions
void draw_image(window_t *window, image_t image, position_t position);

// free functions
void free_image(image_t *image);
void free_shape(shape_t *shape);
void free_text(text_t *text);
void free_polygon(polygon_t *polygon);
void free_ellipse(ellipse_t *ellipse);
void free_circle(circle_t *circle);
void free_rect(rect_t *rect);
void free_window(window_t *window);

// transformations
void translate(position_t *position, int x, int y);
void scale(position_t *position, int x, int y);
void rotate(position_t *position, int angle);


#endif