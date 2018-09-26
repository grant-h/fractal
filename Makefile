all:
	gcc $(shell sdl-config --cflags --libs) -o frac frac.c
