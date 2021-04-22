all:
	gcc $(shell sdl2-config --cflags --libs) -o frac frac.c
