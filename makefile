.PHONY: all win upx test testsoft test60 dbg clean

name = Hyperborea
SHELL := /bin/bash

all:
	cc main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_linux
	strip --strip-unneeded $(name)_linux

win:
	i686-w64-mingw32-gcc -DGLFW main.c rply.c -L. -Ofast -lglfw3dll -lm -o $(name)_windows.exe
	strip --strip-unneeded $(name)_windows.exe

upx:
	upx --lzma --best $(name)_linux
	upx --lzma --best $(name)_windows.exe

test:
	cc -DTEST main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	/tmp/$(name)_test
	rm /tmp/$(name)_test

testsoft:
	cc -DTEST main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	LIBGL_ALWAYS_SOFTWARE=1 /tmp/$(name)_test
	rm /tmp/$(name)_test

test60:
	cc -DTEST main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	xrandr --rate 60 && /tmp/$(name)_test && xrandr --rate 165
	rm /tmp/$(name)_test

dbg:
	cc main.c rply.c -fsanitize=leak -fsanitize=undefined -fsanitize=address -ggdb3 -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_dbg
	$(name)_dbg

clean:
	rm -f $(name)_linux
	rm -f $(name)_windows.exe