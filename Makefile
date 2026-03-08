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

.PHONY: all clean run uilib_demo run_uilib

all: demo

demo: examples/demo.c include/prism.h src/prism.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

UILIB_INCLUDES = -Iinclude -Isrc -Iuilib
ifeq ($(OS),Windows_NT)
    UILIB_LIBS := -lcomdlg32 -lshell32
else
    UILIB_LIBS :=
endif

uilib_demo: uilib/demo.c uilib/uilib.c uilib/uilib.h include/prism.h src/prism.h
	$(CC) $(CFLAGS) $(UILIB_INCLUDES) -o $@ uilib/demo.c uilib/uilib.c $(LIBS) $(UILIB_LIBS)

run: demo
	./demo

run_uilib: uilib_demo
	./uilib_demo

clean:
	$(RM) demo uilib_demo

