.PHONY: all sdl glfw win bsd bsdsdl upx vbo test testsoft test60 dbg grind clean

name = Hyperborea
SHELL := /bin/bash

all: glfw

sdl:
	cc -DSDL_AUDIO main.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_sdl
	strip --strip-unneeded $(name)_sdl

glfw:
	cc -DGLFW -DALSA main.c -Ofast -lglfw -lasound -lpthread -lm -o $(name)_linux
	strip --strip-unneeded $(name)_linux

win:
	i686-w64-mingw32-gcc -DGLFW -DWIN main.c -L. -Ofast -lwinmm -lglfw3dll -lm -o $(name)_windows.exe
	strip --strip-unneeded $(name)_windows.exe

bsd:
	cc -DGLFW -DBSD main.c -I/usr/local/include -L/usr/local/lib -Ofast -lglfw -lpthread -lm -o $(name)_bsd
	strip --strip-unneeded $(name)_bsd

bsdsdl:
	cc -DSDL_AUDIO main.c -I/usr/local/include -L/usr/local/lib -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o $(name)_bsdsdl
	strip --strip-unneeded $(name)_bsdsdl

upx:
	upx --lzma --best $(name)_linux
	upx --lzma --best $(name)_windows.exe

vbo:
	cc -DTEST -DRPLY -DEXPORT_VBO main.c rply.c -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	/tmp/$(name)_test
	rm /tmp/$(name)_test

test:
	cc -DGLFW -DTEST main.c -Ofast -lglfw -lm -o /tmp/$(name)_test
	/tmp/$(name)_test
	rm /tmp/$(name)_test

testsoft:
	cc -DGLFW -DTEST main.c -Ofast -lglfw -lm -o /tmp/$(name)_test
	LIBGL_ALWAYS_SOFTWARE=1 /tmp/$(name)_test
	rm /tmp/$(name)_test

test60:
	cc -DGLFW -DTEST main.c -Ofast -lglfw -lm -o /tmp/$(name)_test
	xrandr --rate 60 && /tmp/$(name)_test && xrandr --rate 165
	rm /tmp/$(name)_test

dbg:
	cc -DGLFW main.c -fsanitize=leak -fsanitize=undefined -fsanitize=address -ggdb3 -lglfw -lm -o $(name)_dbg
	$(name)_dbg

grind:
	valgrind --leak-check=full --leak-check=full ./$(name)_linux

clean:
	rm -f $(name)_sdl
	rm -f $(name)_dbg
	rm -f $(name)_windows.exe
	rm -f $(name)_bsd
	rm -f $(name)_bsdsdl
	rm -f $(name)_glfw
