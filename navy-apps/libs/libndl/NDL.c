#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;

uint32_t NDL_GetTicks() {
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_usec / 1000 + time.tv_sec * 1000;
}

int NDL_PollEvent(char *buf, int len) {
  int read_len = read(3, buf, len);
  close(3);
  return (read_len != 0);
}

void NDL_OpenCanvas(int *w, int *h) {
  char buf[64] = {};
  ssize_t bytes_read = read(4, buf, sizeof(buf) - 1);
  if (bytes_read <= 0) {
    perror("Error reading /dev/dispinfo");
    exit(EXIT_FAILURE);
  }
  buf[bytes_read] = '\0';
  sscanf(buf, "%d %d", &screen_w, &screen_h);

  if (screen_h < *h || *h <= 0) {
    *h = screen_h;
  }
  if (screen_w < *w || *w <= 0) {
    *w = screen_w;
  }
  canvas_h = *h; canvas_w = *w;

  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  uint32_t *ptr = pixels;
  int act_w, act_h;

  act_w = (x + w > canvas_w) ? (canvas_w - x) : w;
  act_h = (y + h > canvas_h) ? (canvas_h - y) : h;

  if (act_w <= 0 || act_h <= 0) {
    return;
  }

  int x_offset = (screen_w - canvas_w) / 2;
  int y_offset = (screen_h - canvas_h) / 2;

  for (int i = 0; i < act_h; i++) {
    uint32_t offset = screen_w * (y + i + y_offset) + (x + x_offset);
    lseek(5, offset * sizeof(uint32_t), SEEK_SET);
    write(5, ptr, act_w * sizeof(uint32_t));
    ptr += w;
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
