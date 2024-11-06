#include <am.h>
#include <nemu.h>
#include <klib.h>

#define AUDIO_FREQ_ADDR      (AUDIO_ADDR + 0x00)
#define AUDIO_CHANNELS_ADDR  (AUDIO_ADDR + 0x04)
#define AUDIO_SAMPLES_ADDR   (AUDIO_ADDR + 0x08)
#define AUDIO_SBUF_SIZE_ADDR (AUDIO_ADDR + 0x0c)
#define AUDIO_INIT_ADDR      (AUDIO_ADDR + 0x10)
#define AUDIO_COUNT_ADDR     (AUDIO_ADDR + 0x14)

#define SBUF_ADDR AUDIO_SBUF_ADDR
#define SBUF_SIZE 0x10000

static uint32_t sbuf_size = 0;
static uint32_t write_pos = 0;

void memcpy_to_mmio(uintptr_t dest, void *src, size_t n) {
  uint8_t *data = (uint8_t *)src;
  for (size_t i = 0; i < n; i++) {
    outb(dest + i, data[i]);
  }
}

void __am_audio_init() {
}

void __am_audio_config(AM_AUDIO_CONFIG_T *cfg) {
  cfg->present = true;
  cfg->bufsize = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_ctrl(AM_AUDIO_CTRL_T *ctrl) {
  outl(AUDIO_FREQ_ADDR, ctrl->freq);
  outl(AUDIO_CHANNELS_ADDR, ctrl->channels);
  outl(AUDIO_SAMPLES_ADDR, ctrl->samples);
  outl(AUDIO_INIT_ADDR, 1); // 初始化
  sbuf_size = inl(AUDIO_SBUF_SIZE_ADDR);
}

void __am_audio_status(AM_AUDIO_STATUS_T *stat) {
  stat->count = inl(AUDIO_COUNT_ADDR);
}

void __am_audio_play(AM_AUDIO_PLAY_T *ctl) {
  uint32_t len = ctl->buf.end - ctl->buf.start;
  uint8_t *data = (uint8_t *)ctl->buf.start;

  while (len > 0) {
    uint32_t free_space = sbuf_size - inl(AUDIO_COUNT_ADDR);
    if (free_space == 0) {
      // 缓冲区已满，等待
      continue;
    }

    uint32_t write_len = (len < free_space) ? len : free_space;
    uint32_t first_chunk = (write_pos + write_len <= sbuf_size) ? write_len : sbuf_size - write_pos;

    memcpy_to_mmio(SBUF_ADDR + write_pos, data, first_chunk);
    if (write_len > first_chunk) {
      memcpy_to_mmio(SBUF_ADDR, data + first_chunk, write_len - first_chunk);
      write_pos = write_len - first_chunk;
    } else {
      write_pos += first_chunk;
    }

    write_pos %= sbuf_size;

    data += write_len;
    len -= write_len;

    // 更新 count 寄存器
    uint32_t current_count = inl(AUDIO_COUNT_ADDR);
    outl(AUDIO_COUNT_ADDR, current_count + write_len);
  }
}