.PHONY: all win bsd upx test testsoft test60 dbg grind clean

name = Hyperborea
SHELL := /bin/bash

all:
	cc -DSDL_AUDIO main.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_linux
	strip --strip-unneeded $(name)_linux

win:
	i686-w64-mingw32-gcc -DGLFW -DWIN main.c -L. -Ofast -lwinmm -lglfw3dll -lm -o $(name)_windows.exe
	strip --strip-unneeded $(name)_windows.exe

bsd:
	cc -DGLFW main.c -I/usr/local/include -L/usr/local/lib -Ofast -lglfw -lm -o $(name)_bsd

upx:
	upx --lzma --best $(name)_linux
	upx --lzma --best $(name)_windows.exe

test:
	cc -DTEST main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	/tmp/$(name)_test
	rm /tmp/$(name)_test

testsoft:
	cc -DTEST main.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	LIBGL_ALWAYS_SOFTWARE=1 /tmp/$(name)_test
	rm /tmp/$(name)_test

test60:
	cc -DTEST main.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	xrandr --rate 60 && /tmp/$(name)_test && xrandr --rate 165
	rm /tmp/$(name)_test

dbg:
	cc main.c -fsanitize=leak -fsanitize=undefined -fsanitize=address -ggdb3 -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_dbg
	$(name)_dbg

grind:
	valgrind --leak-check=full --leak-check=full ./$(name)_linux

clean:
	rm -f $(name)_linux
	rm -f $(name)_dbg
	rm -f $(name)_windows.exe
