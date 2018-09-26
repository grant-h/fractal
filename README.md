## Mandelbrot Zoomer

A mandelbrot set viewer I made to play with SDL and complex numbers.
The only requirement is libSDL (version 1, not 2).

![Mandelbrot](/img/demo.png?raw=true)

### Running

```
fractal/ $ make
gcc -I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_THREAD_SAFE -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa -o frac frac.c
fractal/ $ ./frac
```
