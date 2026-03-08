## Prism

![Logo](assets/icon.png)

Prism is a small cross‑platform 2D graphics library written in C. it includes a UI Library

It provides:

- **Software‑rendered framebuffers** backed by a simple `int *pixels` array.
- **Window creation and presentation** for Windows (Win32) and Linux (X11), with a portable API that also compiles on macOS.
- **Basic drawing primitives**: pixels, lines, rectangles, circles, ellipses, polygons, and generic shapes.
- **Input helpers** for keyboard and mouse
- **UI Library** for small quick prototypes

Prism is designed to be:

- **Minimal**: just C and the OS APIs; no external libraries.
- **Embeddable**: the entire implementation currently lives in `src/prism.h`.
- **Portable**: a single codebase that targets Windows and Linux today, with room for a macOS backend.

---

## Getting started

### Requirements

- **Compiler**: Any reasonably modern C compiler with C11 support.
  - Windows: MSVC, clang-cl, or MinGW gcc/clang.
  - Linux: gcc or clang.
- **System libraries**:
  - Windows: links against `user32` and `gdi32`.
  - Linux: links against `X11` and `m` (math).

### Building the demo

From the project root:

- **Linux**

```bash
make && make uilib_demo
./demo
./uilib_demo
```

This uses the provided `Makefile`, which compiles `examples/demo.c` and `uilib/demo.c` and links against X11.

- **Windows (MinGW / clang / other make‑based toolchains)**

```bash
make && make uilib_demo
demo.exe
uilib_demo.exe
```

The `Makefile` detects `OS=Windows_NT` and links against `user32` and `gdi32`.  
If you use MSVC directly, you can build with:

```bash
cl /std:c11 /W4 /Iinclude /Isrc examples\demo.c user32.lib gdi32.lib
demo.exe
```
---

### Notes

This project is a mix of vibe-coding and manual coding, its a testimony to how good vibecoding have gotten and can get with the help of human support and good prompting.