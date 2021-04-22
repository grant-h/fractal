#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define AA_LEVEL 4
#define abs(x) ((x > 0) ? (x) : -(x))

struct frac_bounds
{
  double r_min, r_max;
  double i_min, i_max;
};

struct color
{
  Uint8 r, g, b;
};

unsigned int iterations = 50;
struct frac_bounds bounds;
bool redraw = false;
bool aa_active = false;

void frac_reset();
void frac_iter_inc();
void frac_iter_dec();
void frac_get_coord(unsigned short x, unsigned short y, double * r, double * i);
void frac_center_at(unsigned short x, unsigned short y);
void frac_zoom(unsigned short x, unsigned short y, double zoom);
void frac_zbox(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2);
struct color color_fade(struct color color, double fadeVal);
struct color color_inter(struct color l, struct color r, double val);
Uint32 frac_map_palette(SDL_Surface * s, int escape);

void draw_frac(SDL_Surface * surf);
void draw_frac_aa(SDL_Surface * surf);

int main (int argc, char * argv[])
{
    // initialize SDL video
    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "Unable to init SDL: %s\n", SDL_GetError() );
        return 1;
    }

    // make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // create a new window
    /*SDL_Surface* screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16,
                                           SDL_HWSURFACE|SDL_DOUBLEBUF);*/
    // create a new window
    SDL_Window *window = SDL_CreateWindow("Fractal",
                          SDL_WINDOWPOS_UNDEFINED,
                          SDL_WINDOWPOS_UNDEFINED,
                          SCREEN_WIDTH, SCREEN_HEIGHT,
                          0);
    SDL_Surface *screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                        0x00FF0000,
                                        0x0000FF00,
                                        0x000000FF,
                                        0xFF000000);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                            SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING,
                                            SCREEN_WIDTH, SCREEN_HEIGHT);
    if ( !screen )
    {
        printf("Unable to set 640x480 video: %s\n", SDL_GetError());
        return 1;
    }

    frac_reset();

    struct timeval start;
    int fps = 0;
    int done = 0;
    bool dragging = false, leftDown = false;
    int leftSaveX, leftSaveY;

    gettimeofday(&start, NULL);

    while(!done)
    {
        // message processing loop
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // check for messages
            switch (event.type)
            {
                // exit if the window is closed
            case SDL_QUIT:
                done = true;
                break;
                // check for keypresses
            case SDL_KEYDOWN:
                {
                  // exit if ESCAPE is pressed
                  if(event.key.keysym.sym == SDLK_ESCAPE)
                    done = true;
                  if(event.key.keysym.sym == SDLK_r)
                    frac_reset(); 
                  if(event.key.keysym.sym == SDLK_o)
                    frac_iter_inc();
                  if(event.key.keysym.sym == SDLK_p)
                    frac_iter_dec();
                  if(event.key.keysym.sym == SDLK_a)
                  {
                    redraw = true;
                    aa_active = !aa_active; 
                  }
                  break;
                }
            case SDL_MOUSEBUTTONDOWN:
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                  leftDown = true;
                  leftSaveX = event.button.x;
                  leftSaveY = event.button.y;
                }
                else if(event.button.button == SDL_BUTTON_RIGHT)
                {
                  double r, i;

                  frac_get_coord(event.button.x, event.button.y, &r, &i);

                  printf("(%10.10lf, %10.10lf)\n", r, i);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if(event.button.button == SDL_BUTTON_LEFT)
                {
                  if(dragging)
                    frac_zbox(leftSaveX, leftSaveY, event.button.x, event.button.y);
                  else
                    frac_zoom(leftSaveX, leftSaveY, 0.1);

                  leftDown = false;
                  dragging = false;
                }
            } // end switch
        } // end of message processing

        ///////////////////
        //TIMING START
        ///////////////////

        if(redraw)
        {
          if(SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0) 
              return 1;

          //clear screen
          SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

          if(aa_active)
            draw_frac_aa(screen);
          else
            draw_frac(screen);

          if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

          SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
          SDL_RenderClear(renderer);
          SDL_RenderCopy(renderer, texture, NULL, NULL);
          SDL_RenderPresent(renderer);

          redraw = false;
        }
        else
        {
          SDL_Delay(10);
        }

        fps++;

        ///////////////////
        //TIMING STOP
        ///////////////////

        struct timeval cur, delta;
        gettimeofday(&cur, NULL);

        delta.tv_sec = cur.tv_sec - start.tv_sec;
        delta.tv_usec = cur.tv_usec - start.tv_usec;

        if(delta.tv_sec >= 1)
        {
          printf("FPS %d\n", fps); 
          fps = 0;
          gettimeofday(&start, NULL);
        }

    } // end main loop

    printf("Exited cleanly\n");
    return 0;
}

void frac_reset()
{
  bounds.r_min = -1.0;
  bounds.r_max = 1.0;
  bounds.i_min = -1.0;
  bounds.i_max = bounds.i_min + (bounds.r_max-bounds.r_min)*(double)SCREEN_HEIGHT/SCREEN_WIDTH;

  redraw = true;
}

void frac_iter_inc()
{
  iterations += 20;

  printf("Iterations: %d\n", iterations);

  redraw = true;
}

void frac_iter_dec()
{
  if(iterations > 30)
  {
    iterations -= 20;
    printf("Iterations: %d\n", iterations);

    redraw = true;
  }
}

void frac_get_coord(unsigned short x, unsigned short y, double * r, double * i)
{
  if(i && r)
  {
    *r = bounds.r_min + abs(bounds.r_max-bounds.r_min)*((double)x/SCREEN_WIDTH);
    *i = bounds.i_min + abs(bounds.i_max-bounds.i_min)*((double)y/SCREEN_HEIGHT);
  }
}

void frac_center_at(unsigned short x, unsigned short y)
{
  double r, i;

  frac_get_coord(x, y, &r, &i);

  double r_min_dist = abs(bounds.r_min-r);
  double r_max_dist = abs(bounds.r_max-r);
  double r_adj = abs(r_min_dist-r_max_dist)/2;

  if(r_min_dist > r_max_dist)
  {
    bounds.r_min += r_adj;
    bounds.r_max += r_adj;
  }
  else if(r_max_dist > r_min_dist)
  {
    bounds.r_min -= r_adj;
    bounds.r_max -= r_adj;
  }

  double i_min_dist = abs(bounds.i_min-i);
  double i_max_dist = abs(bounds.i_max-i);
  double i_adj = abs(i_min_dist-i_max_dist)/2;

  if(i_min_dist > i_max_dist)
  {
    bounds.i_min -= i_adj;
    bounds.i_max -= i_adj;
  }
  else if(i_max_dist > i_min_dist)
  {
    bounds.i_min += i_adj;
    bounds.i_max += i_adj;
  }

  redraw = true;
}

void frac_zoom(unsigned short x, unsigned short y, double zoom)
{
  frac_center_at(x, y);

  //perform magnification
  double r_dist = abs(bounds.r_min-bounds.r_max);
  double r_adj = (r_dist/2)*zoom;
  double i_dist = abs(bounds.i_min-bounds.i_max);
  double i_adj = (i_dist/2)*zoom;

  bounds.r_min += r_adj; 
  bounds.r_max -= r_adj;
  bounds.i_min += i_adj;
  bounds.i_max -= i_adj;

  redraw = true;
}

void frac_zbox(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
  // arrange x* and y* with x1 and y1 being the top left
  // and x2 and y2 being the bottom right
  

}

struct color color_fade(struct color color, double fadeVal)
{
  struct color newColor = color;

  newColor.r *= fadeVal;
  newColor.g *= fadeVal;
  newColor.b *= fadeVal;

  return newColor;
}

struct color color_inter(struct color l, struct color r, double val)
{
  struct color newColor = l;

  int rd = abs(l.r - r.r);
  int gd = abs(l.g - r.g);
  int bd = abs(l.b - r.b);

  Uint8 dr = rd*val,
        dg = gd*val,
        db = bd*val;

  if(l.r > r.r)
    newColor.r -= dr;
  else
    newColor.r += dr;

  if(l.g > r.g)
    newColor.g -= dg;
  else
    newColor.g += dg;

  if(l.b > r.b)
    newColor.b -= db;
  else
    newColor.b += db;

  return newColor;
}

Uint32 frac_map_palette(SDL_Surface * s, int escape)
{
  int width = 15;
  int numColors = 3;
  struct color colors[3] = {{255,0,0}, {0,255,0}, {0,0,255}};

  int colorPos = (escape / width) % numColors;
  double fade = (double)(escape %  width) / width;

  //struct color newColor = color_fade(colors[colorPos], fade);
  struct color newColor = color_inter(colors[colorPos], colors[(colorPos+1) % numColors], fade);

  return SDL_MapRGB(s->format, newColor.r, newColor.g, newColor.b); 
}

void draw_frac(SDL_Surface * surf)
{
  Uint32 *fbuf = surf->pixels;

  double MinRe = bounds.r_min;
  double MaxRe = bounds.r_max;
  double MinIm = bounds.i_min;
  double MaxIm = bounds.i_max;
  //factors scale the x and y positions to the imaginary plane
  double Re_factor = (MaxRe-MinRe)/(SCREEN_WIDTH-1);
  double Im_factor = (MaxIm-MinIm)/(SCREEN_HEIGHT-1);

  unsigned MaxIterations = iterations;
  unsigned y,x,n;

  for(y=0; y<SCREEN_HEIGHT; ++y)
  {
      for(x=0; x<SCREEN_WIDTH; ++x)
      {
          double c_im = MaxIm - y*Im_factor;
          double c_re = MinRe + x*Re_factor;

          double Z_re = c_re, Z_im = c_im;
          bool isInside = true;

          for(n=0; n<MaxIterations; ++n)
          {
              double Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;

              if(Z_re2 + Z_im2 > 4)
              {
                  isInside = false;
                  break;
              }

              Z_im = 2*Z_re*Z_im + c_im;
              Z_re = Z_re2 - Z_im2 + c_re;
          }

          if(isInside) 
            *fbuf++ = SDL_MapRGB(surf->format, 0, 0, 0); 
          else
            *fbuf++ = frac_map_palette(surf, n);
      }
  }
}

void draw_frac_aa(SDL_Surface * surf)
{
  Uint32 *fbuf = surf->pixels;

  double MinRe = bounds.r_min; 
  double MaxRe = bounds.r_max;
  double MinIm = bounds.i_min;
  double MaxIm = bounds.i_max; 
  double Re_factor = (MaxRe-MinRe)/(SCREEN_WIDTH*AA_LEVEL-1);
  double Im_factor = (MaxIm-MinIm)/(SCREEN_HEIGHT*AA_LEVEL-1);

  unsigned MaxIterations = iterations;
  unsigned y,x,n;

  for(y=0; y<SCREEN_HEIGHT; ++y)
  {
      for(x=0; x<SCREEN_WIDTH; ++x)
      {
          unsigned super_y, super_x;
          unsigned startX = x*AA_LEVEL;
          unsigned startY = y*AA_LEVEL;
          unsigned iterAve = 0;

          for(super_y=startY; super_y < startY+AA_LEVEL; super_y++)
          {
            double c_im = MaxIm - super_y*Im_factor;

            for(super_x=startX; super_x < startX+AA_LEVEL; super_x++)
            {
              double c_re = MinRe + super_x*Re_factor;
              double Z_re = c_re, Z_im = c_im;

              for(n=0; n<MaxIterations; ++n)
              {
                  double Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
                  if(Z_re2 + Z_im2 > 4)
                      break;

                  Z_im = 2*Z_re*Z_im + c_im;
                  Z_re = Z_re2 - Z_im2 + c_re;
              }

              iterAve += n;
            }
          }

          //calculate average iter
          iterAve = ((double)iterAve/(AA_LEVEL*AA_LEVEL));

          if(iterAve < MaxIterations)
            *fbuf++ = frac_map_palette(surf, n);
          else
            *fbuf++ = SDL_MapRGB(surf->format, 0, 0, 0);
      }
  }
}
