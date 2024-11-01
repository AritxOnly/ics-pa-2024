#include <am.h>
#include <nemu.h>
#include <string.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
  /* test code */
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (int i = 0; i < w * h; i++) {
    fb[i] = i;
  }
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint16_t w = (inl(VGACTL_ADDR) >> 16) & 0xffff;
  uint16_t h = inl(VGACTL_ADDR) & 0xffff;
  uint32_t sz = w * h * sizeof(uint32_t);
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = sz
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  uint16_t w = (inl(VGACTL_ADDR) >> 16) & 0xffff;

  if (!ctl->pixels || ctl->w == 0 || ctl->h == 0) return; 

  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t *pixels = ctl->pixels;  // 头指针

  for (int y = 0; y < ctl->h; y++) {
    memcpy(&fb[w * (ctl->y + y) + ctl->x], &pixels[ctl->w * y], ctl->w * sizeof(uint32_t));
  }

  if (ctl->sync) {
    outl(SYNC_ADDR, 1); // 写入sync寄存器
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
