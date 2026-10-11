// written by tmids manually
// www.tmidsisthebest.org
// prewritten code for sdl programs so that i wouldnt need to rewrite the same thing over and over again
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

struct Image {
  SDL_Surface *surface;
  //  size_t w, h;
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
void skipCommentsPPM(struct Program *program, FILE *file);
void loadPPM(struct Program *program, char *filename, FILE *file);
void drawPPMASCIIToSurface(struct Program *program, FILE *file, char format);
void drawPPMRawToSurface(struct Program *program, FILE *file, char format);
void SDL_Shutdown(struct Program *program, int exit_code);
int SDL_MyInit(struct Program *program);
int SDL_InitWindow(struct Program *program, char WINDOW_TITLE[256], size_t WINDOW_WIDTH, size_t WINDOW_HEIGHT);
void drawPixel(SDL_Surface *surface, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
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

int SDL_InitWindow(struct Program *program, char WINDOW_TITLE[256], size_t WINDOW_WIDTH, size_t WINDOW_HEIGHT) {
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

void skipCommentsPPM(struct Program *program, FILE *file) {
  int buffer = fgetc(file);
  while (buffer!=EOF) {
    switch (buffer) {
    case ' ': case '\n': case '\t': case '\r':
      buffer = fgetc(file);
      break;
    case '#':
      while ((buffer = fgetc(file))!='\n') {}
      buffer = fgetc(file);
      break;
    default:
      ungetc(buffer, file);
      return;
    }
  }
  SDL_Shutdown(program, EXIT_FAILURE);
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
       if (program->image.zoom<=1) {program->image.zoom = 1;}
       if (program->image.zoom>=255) {program->image.zoom = 255;}
       break;
     }
     }
   }
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

void drawPixel(SDL_Surface *surface, size_t x, size_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
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
  for (size_t desty = 1; desty<h; desty++) {
    for (size_t destx = 1; destx<w; destx++) { 
      srcx = program->image.surface->w*destx/w; // srcx/program->image.surface->w == destx/w
      srcy = program->image.surface->h*desty/h;
      SDL_GetRGBA(getPixel(program->image.surface, srcx, srcy), program->image.surface->format, &r, &g, &b, &a);
      drawPixel(window, destx, desty, r, g, b, a);
    }
  }
  SDL_UpdateWindowSurface(program->window);
}

void loadPPM(struct Program *program, char *filename, FILE *file) {
  struct Image *image = &program->image;
  char format;
  char buffer;
  uint32_t w, h;
  uint16_t d;

  skipCommentsPPM(program, file);

  fscanf(file, "%*c%c", &format);
  //can't imagine a better way than to just slap this inbeetwen every file action
  skipCommentsPPM(program, file);
  
  fscanf(file, "%d", &w);
  skipCommentsPPM(program, file);
  
  if (format!='1'||format!='4') {
    fscanf(file, "%d", &h);
    skipCommentsPPM(program, file);
  }

  printf("w,h:%d,%d\n", w, h);

  fscanf(file, "%hu", &d);
  skipCommentsPPM(program, file);
 
  image->depth = d;
  //image->w = w;
  //image->h = h;
  image->surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_RGBA8888);

  switch (format) {
  case '1': case '2': case '3':
    drawPPMASCIIToSurface(program, file, format);
    break;
  case '4': case '5': case '6':
    drawPPMRawToSurface(program, file, format);
    break;
  }
}

 void drawPPMRawToSurface(struct Program *program, FILE *file, char format) {
printf("loading ppm in p%c format\n", format);
  uint8_t r, g, b;
  uint8_t a = 255;
  size_t w = program->image.surface->w;
  size_t h = program->image.surface->h;
  for (size_t y = 1; y<=h; y++) {
    for (size_t x = 1; x<=w; x++) {
      switch (format) {
      case '4': {
	fread(&r, 8, 1, file);
	r *= 255;
	printf("color value: %d\n", r);
	g = r; b = r;
	break;
      }
     case '5': {
 	break;
      }
      case '6': {
	break;
	}
      }
      drawPixel(program->image.surface, x, y, r, g, b, a);
    }
  }  
}

void drawPPMASCIIToSurface(struct Program *program, FILE *file, char format) {
  printf("loading ppm in p%c format\n", format);
  uint16_t r, g, b;
  uint8_t a = 255;
  int32_t w = program->image.surface->w;
  int32_t h = program->image.surface->h;
  for (size_t y = 1; y<=h; y++) {
    for (size_t x = 1; x<=w; x++) {
      switch (format) {
      case '1': {
	skipCommentsPPM(program, file);
	fscanf(file, "%hu", &r);
	r *= 255;
	g = r; b = r;
	break;
      }
     case '2': {
	skipCommentsPPM(program, file);
 	fscanf(file, "%hu", &r);
	g = r; b = r;
	break;
      }
      case '3': {
	skipCommentsPPM(program, file);
	fscanf(file, "%hu", &r);
	skipCommentsPPM(program, file);
	fscanf(file, "%hu", &g);
	skipCommentsPPM(program, file);
	fscanf(file, "%hu", &b);
	break;
	}
      }
      drawPixel(program->image.surface, x, y, r, g, b, a);
    }
  }
}

void loadImage(struct Program *program, char *filename, FILE *file) {
  char *suffix = strrchr(filename, '.')+1;
  
  if (!strcmp(suffix, "ppm") || !strcmp(suffix, "pbm") || !strcmp(suffix, "pgm")) {
    loadPPM(program, filename, file);
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

  FILE *file = fopen(argV[1], "rb");
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

