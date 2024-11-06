/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include <pthread.h>

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

static SDL_AudioSpec sdl_spec = {};   // SDL 音频规格
static uint32_t sbuf_size = CONFIG_SB_SIZE;  // 缓冲区大小
static uint32_t count = 0;            // 缓冲区中数据的字节数
static uint32_t read_pos = 0;         // 读取位置

static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;

static void audio_callback(void *userdata, Uint8 *stream, int len) {
  int remain = len;

  while (remain > 0) {
    pthread_mutex_lock(&audio_mutex);
    int available = (count < remain) ? count : remain;
    pthread_mutex_unlock(&audio_mutex);

    if (available > 0) {
      if (read_pos + available <= sbuf_size) {
        memcpy(stream + (len - remain), sbuf + read_pos, available);
        read_pos += available;
      } else {
        int first_chunk = sbuf_size - read_pos;
        memcpy(stream + (len - remain), sbuf + read_pos, first_chunk);
        memcpy(stream + (len - remain) + first_chunk, sbuf, available - first_chunk);
        read_pos = available - first_chunk;
      }

      read_pos %= sbuf_size;

      pthread_mutex_lock(&audio_mutex);
      count -= available;
      pthread_mutex_unlock(&audio_mutex);
    } else {
      memset(stream + (len - remain), 0, remain);
      remain = 0;
      break;
    }

    remain -= available;
  }
}

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (is_write) {
    uint32_t reg_index = offset / 4;
    uint32_t data = *(uint32_t *)((uint8_t *)audio_base + offset);

    switch (reg_index) {
      case reg_freq:
        sdl_spec.freq = data;
        break;
      case reg_channels:
        sdl_spec.channels = data;
        break;
      case reg_samples:
        sdl_spec.samples = data;
        break;
      case reg_init:
        if (data) {
          sdl_spec.format = AUDIO_S16SYS;  // 16 位有符号系统字节序
          sdl_spec.callback = audio_callback;
          sdl_spec.userdata = NULL;

          if (SDL_OpenAudio(&sdl_spec, NULL) < 0) {
            panic("SDL_OpenAudio: %s", SDL_GetError());
          }

          SDL_PauseAudio(0);  // 开始播放音频
          audio_base[reg_sbuf_size] = sbuf_size;
        }
        break;
      default:
        break;
    }
  } else {
    uint32_t reg_index = offset / 4;
    switch (reg_index) {
      case reg_count:
        pthread_mutex_lock(&audio_mutex);
        audio_base[reg_count] = count;
        pthread_mutex_unlock(&audio_mutex);
        break;
      case reg_sbuf_size:
        audio_base[reg_sbuf_size] = sbuf_size;
        break;
      default:
        break;
    }
  }
}

void init_audio() {
  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
    panic("Failed to initialize SDL: %s", SDL_GetError());
  }

  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, NULL);
}