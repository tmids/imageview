// written by tmids manually
// www.tmidsisthebest.org
// prewritten code for sdl programs so that i wouldnt need to rewrite the same thing over and over again
#include <SDL2/SDL.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct Image {
  SDL_Surface *surface;
  uint16_t depth;
  uint8_t zoom;
  char format[20];
  char title[256];
};

struct Program {
  SDL_Window *window;
  //SDL_Renderer *renderer;
  struct Image image;
};


void loadImage(struct Program *program, char *filename, FILE *file);
void skipCommentsPPM(FILE *file);
void loadPPM(struct Program *program, FILE *file);
void loadPPMP3(struct Program *program, FILE *file);
void SDL_Shutdown(struct Program *program, int exit_code);
int SDL_MyInit(struct Program *program);
int SDL_InitWindow(struct Program *program, char WINDOW_TITLE[256], int WINDOW_WIDTH, int WINDOW_HEIGHT);
void drawPixel(SDL_Surface *surface, uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b, uint32_t a);
void updateWindow(struct Program *program);

// Source - https://stackoverflow.com/a/53067795
// Posted by Danny
// Retrieved 2026-10-03, License - CC BY-SA 4.0

Uint32 getPixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

switch (bpp)
{
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(Uint32 *)p;
            break;

        default:
            return 0;       /* shouldn't happen, but avoids warnings */
      }
}

int SDL_InitWindow(struct Program *program, char WINDOW_TITLE[256], int WINDOW_WIDTH, int WINDOW_HEIGHT) {
   program->window =
       SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
   if (!program->window) {
     printf("didnt create window. %s\n", SDL_GetError());
     return 1;
   }

   return 0;
}

int SDL_MyInit(struct Program *program) {
   if (SDL_Init(SDL_INIT_EVERYTHING)) {
     printf("didnt initialise sdl. %s\n", SDL_GetError());
     return 1;
   };
   /*
   program->renderer = SDL_CreateRenderer(program->window, -1, 0);
   if (!program->renderer) {
     printf("didnt create renderer. %s", SDL_GetError());
     return 1;
     }*/

   return 0;
 }

void skipCommentsPPM(FILE *file) {
  char buffer;
  while (buffer=='#') {
    while (buffer=='\n') {
      buffer = fgetc(file);
    }
  }
}

void event_handler(struct Program *program) {
   SDL_Event event;
   while (SDL_PollEvent(&event)) {
     switch (event.type) {
     case SDL_QUIT: {
       SDL_Shutdown(program, EXIT_SUCCESS);
       break;
     }
     case SDL_MOUSEWHEEL: {
       uint8_t scroll = event.wheel.y;
       program->image.zoom -= scroll;    
     }
     }
   }
   if (program->image.zoom<=1) {program->image.zoom = 1;}
   if (program->image.zoom>=255) {program->image.zoom = 255;}
   SDL_SetWindowSize(program->window, program->image.surface->w*program->image.zoom/100, program->image.surface->h*program->image.zoom/100);
 }

 void SDL_Shutdown(struct Program *program, int exit_status) {
   //   SDL_DestroyRenderer(program->renderer);
   if (program->window) {
     SDL_DestroyWindow(program->window);
   }
   SDL_Quit();
   exit(exit_status);
 }

void drawPixel(SDL_Surface *surface, uint32_t x, uint32_t y, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
  SDL_FillRect(surface, &(SDL_Rect){.x=x,.y=y,.w=1,.h=1,}, SDL_MapRGBA(surface->format, r, g, b, a));
}

void updateWindow(struct Program *program) {
  SDL_Surface *window = SDL_GetWindowSurface(program->window);
  int32_t w, h, srcx, srcy;
  uint8_t r, g, b, a;
  //SDL_GetWindowSize(program->window, &w, &h);
  w = window->w;
  h = window->h;
  SDL_FillRect(window, NULL, SDL_MapRGBA(window->format, 0, 0, 0, 255));
  for (int desty = 1; desty<h; desty++) {
    for (int destx = 1; destx<w; destx++) { 
      srcx = program->image.surface->w*destx/w; // srcx/program->image.surface->w == destx/w
      srcy = program->image.surface->h*desty/h;
      SDL_GetRGBA(getPixel(program->image.surface, srcx, srcy), program->image.surface->format, &r, &g, &b, &a);
      drawPixel(window, destx, desty, r, g, b, a);
    }
  }
  SDL_UpdateWindowSurface(program->window);
}

void loadPPM(struct Program *program, FILE *file) {
  struct Image *image = &program->image;
  char format[3];
  char buffer;
  uint32_t w, h;
  uint16_t d;

  skipCommentsPPM(file);
  
  fgets(format, 3, file);

  //can't imagine a better way than to just slap this inbeetwen every file action
  skipCommentsPPM(file);
    
  fscanf(file, "%d %d", &w, &h);
  
  skipCommentsPPM(file);

  printf("w,h:%d,%d\n", w, h);

  fscanf(file, "%hu", &d);

  skipCommentsPPM(file);
 
  image->depth = d;
  image->surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_RGBA8888);

  if (!strcmp(&format[1], "3")) {
    printf("loading ppm\n");
    loadPPMP3(program, file);
  } else {
    printf("Unsupported PPM format! %s\n", format);
  } 
}

void loadPPMP3(struct Program *program, FILE *file) {
  printf("loading ppm in p3 format\n");
  int16_t r, g, b;
  uint8_t a = 255;
  int32_t w = program->image.surface->w;
  int32_t h = program->image.surface->h;
  for (int y = 1; y<=h; y++) {
    for (int x = 1; x<=w; x++) {
      fscanf(file, "%hu %hu %hu", &r, &g, &b);
      drawPixel(program->image.surface, x, y, r, g, b, a);
    }
  }
}

void loadImage(struct Program *program, char *filename, FILE *file) {
  char *suffix = strrchr(filename, '.')+1;
  
  if (!strcmp(suffix, "ppm") || !strcmp(suffix, "pbm") || !strcmp(suffix, "pgm")) {
    loadPPM(program, file);
  } else {
    printf("unsupported image type! %s\n", suffix);
  }
}

int main(int argC, char *argV[]) {
  struct Program program = {
    .window = NULL,
    //    .renderer = NULL,    
    .image = {.zoom=100,}
  };

  FILE *file = fopen(argV[1], "r");
  if (file==NULL) {
    printf("Couldn't open file! %s\n", argV[1]);
    SDL_Shutdown(&program, EXIT_FAILURE);
  }
  char path[256];
  strcpy(path, argV[1]);
  char *filename = strrchr(path, '/');
  if (filename==NULL) {filename=&path[0];}
  else {filename+=1;}
  strcpy(program.image.title, filename);
  
  if (SDL_MyInit(&program)) {
    SDL_Shutdown(&program, EXIT_FAILURE);
  }
  
  loadImage(&program, filename, file);

  if (SDL_InitWindow(&program, program.image.title, program.image.surface->w, program.image.surface->h)) {
    SDL_Shutdown(&program, EXIT_FAILURE);
  }
  
  updateWindow(&program);
  
  while (1) {
    event_handler(&program);
    updateWindow(&program);
  }
  
  return 0;
}

