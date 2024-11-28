#include <NDL.h>
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int SDL_PushEvent(SDL_Event *ev) {
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  return 0;
}

int SDL_WaitEvent(SDL_Event *event) {
  char buf[64];
  while (1) {
    if (NDL_PollEvent(buf, sizeof(buf))) {
      char keydown, key[32];
      sscanf(buf, "k%c %s", &keydown, key);

      event->type = (keydown == 'd') ? SDL_KEYDOWN : SDL_KEYUP;

      for (int i = 0; i < KEY_NUMS; i++) {
        if (strcmp(key, keyname[i]) == 0) {
          event->key.keysym.sym = i;
          break;
        }
      }
      return 1;
    }
  }
  return 0;
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  return NULL;
}
