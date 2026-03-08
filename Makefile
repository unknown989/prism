CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -pedantic
INCLUDES = -Iinclude -Isrc

ifeq ($(OS),Windows_NT)
    TARGET_OS := windows
    LIBS := -luser32 -lgdi32
else
    TARGET_OS := linux
    LIBS := -lX11 -lm
endif

.PHONY: all clean run

all: demo

demo: examples/demo.c include/prism.h src/prism.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

run: demo
	./demo

clean:
	$(RM) demo

