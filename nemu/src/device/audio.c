/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint32_t *audio_base = NULL;

struct Stream_Buf {
  uint32_t start;
  uint32_t end;
} sbuf_area = { .start = 0 };

static SDL_AudioSpec audio_spec = {}; // 音频信息

void audio_callback(void *userdata, uint8_t *stream, int len) {
  SDL_memset(stream, 0, len); // 清空流缓冲区，防止存在杂音
  uint32_t count = audio_base[reg_count]; // 当前流缓冲区的大小
  if (count == 0) return;
  int actual_len = (len > count) ? count : len;
  // 写入流缓冲区
  SDL_memcpy(stream, sbuf + sbuf_area.start, actual_len);
  sbuf_area.start = (sbuf_area.start + actual_len) % audio_base[reg_sbuf_size];
  audio_base[reg_count] -= actual_len;
}

static void audio_system_init() {
  audio_spec.format = AUDIO_S16SYS;
  audio_spec.userdata = NULL;
  audio_spec.freq = audio_base[reg_freq];
  audio_spec.channels = audio_base[reg_channels];
  audio_spec.samples = audio_base[reg_samples];
  audio_spec.callback = audio_callback;

  // 初始化音频子系统
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
    panic("Audio Sub-syetem initialization failed!!!");
  }
  // 打开音频设备
  if (SDL_OpenAudio(&audio_spec, NULL) != 0) {
    panic("SDLOpenAudio failed!!!");
  }
  SDL_PauseAudio(0);
  audio_base[reg_init] = 0;
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  uint32_t reg_idx = offset / 4;
  if (is_write) {
    switch (reg_idx) {
      case reg_init:
      case reg_freq:
      case reg_samples:
      case reg_channels:
        audio_system_init();
        break;
      default: break;
    }
  }
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);

  audio_base[reg_sbuf_size] = CONFIG_SB_SIZE;
  sbuf_area.end = audio_base[reg_sbuf_size];
}