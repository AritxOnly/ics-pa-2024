#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>
#include <assert.h>

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  FILE* fp = fopen(filename, "r");
  if (!fp) {
    fprintf(stderr, "Failed to open %s\n", filename);
    return NULL;
  }
  if (fseek(fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "Failed to read %s\n", filename);
    fclose(fp);
    return NULL;
  }
  long filesz = ftell(fp);
  if (filesz <= 0)  {
    fprintf(stderr, "Failed to get filesz = %d\n", (int)filesz);
    return NULL;
  }

  uint8_t *buf = malloc(filesz);
  if (!buf) {
    fprintf(stderr, "Failed to allocate buffer\n");
    return NULL;
  }
  fseek(fp, 0, SEEK_SET);
  int rd = fread(buf, 1, filesz, fp);
  assert(rd == filesz);
  SDL_Surface *s = STBIMG_LoadFromMemory(buf, filesz);
  fclose(fp);
  return s;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
