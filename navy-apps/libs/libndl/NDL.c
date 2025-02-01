#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

static int evtdev = -1;
static int fbdev = -1;
static int sbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_w = 0, canvas_h = 0;

uint32_t NDL_GetTicks() {
  struct timeval time;
  gettimeofday(&time, NULL);
  return time.tv_usec / 1000 + time.tv_sec * 1000;
}

int NDL_PollEvent(char *buf, int len) {
  int fd = open("/dev/events", O_RDONLY);
  int read_len = read(fd, buf, len);
  close(fd);
  return (read_len != 0);
}

void NDL_OpenCanvas(int *w, int *h) {
  int fd = open("/proc/dispinfo", O_RDONLY);
  char buf[64] = {};
  ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
  if (bytes_read <= 0) {
    perror("Error reading /dev/dispinfo");
    exit(EXIT_FAILURE);
  }
  buf[bytes_read] = '\0';
  sscanf(buf, "WIDTH:%d\nHEIGHT:%d", &screen_w, &screen_h);
  // printf("[%d, %d]", screen_w, screen_h);
  close(fd);

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
  int fd = open("/dev/fb", O_WRONLY);
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
    lseek(fd, offset * sizeof(uint32_t), SEEK_SET);
    write(fd, ptr, act_w * sizeof(uint32_t));
    ptr += w;
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
  int fd = open("/dev/sbctl", O_WRONLY);
  int sbinfo[] = { freq, channels, samples };
  write(fd, sbinfo, 3 * sizeof(int));
  close(fd);

  sbdev = open("/dev/sb", O_WRONLY);
  assert(sbdev >= 0);
}

void NDL_CloseAudio() {
  if (sbdev >= 0) {
    close(sbdev);
    sbdev = -1;
  }
}

int NDL_PlayAudio(void *buf, int len) {
  if (sbdev < 0) {
    return 0;
  }

  ssize_t write_len = write(sbdev, buf, len);
  assert(write_len >= 0);
  return write_len;
}

int NDL_QueryAudio() {
  int fd = open("/dev/sbctl", O_RDONLY);

  int free = 0;
  ssize_t read_len = read(fd, &free, sizeof(int));
  assert(read_len == sizeof(int));

  close(fd);

  return free;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  return 0;
}

void NDL_Quit() {
}
