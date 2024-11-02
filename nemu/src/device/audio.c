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

static SDL_AudioSpec sdl_spec = {}; // 存储音频的结构体
static uint32_t sbuf_size = CONFIG_SB_SIZE;   // 地址
static uint32_t count = 0;  // 数据量
static uint32_t read_pos = 0; // 读取位置
static SDL_AudioDeviceID device_id = 0; // 设备ID

static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  if (is_write) { // 写
    uint32_t reg_index = offset / 4;  // 下标
    uint32_t data = *(uint32_t *)((uint8_t *)audio_base + offset);

    switch (reg_index) {
      case reg_freq: sdl_spec.freq = data; break;
      case reg_channels: sdl_spec.channels = data; break;
      case reg_samples: sdl_spec.samples = data; break;
      case reg_init:
        if (data) {
          sdl_spec.format = AUDIO_S16SYS;
          sdl_spec.callback = NULL;

          device_id = SDL_OpenAudioDevice(NULL, 0, &sdl_spec, NULL, 0);
          Assert(device_id, "SDL_OpenAudioDevice: %s", SDL_GetError());

          SDL_PauseAudioDevice(device_id, 0);
          audio_base[reg_sbuf_size] = sbuf_size;
        }
        break;
      default:
        break;
    }
  } else {  // 读
    uint32_t reg_index = offset / 4;
    switch (reg_index) {
      case reg_count: audio_base[reg_count] = count; break;
      case reg_sbuf_size: audio_base[reg_sbuf_size] = sbuf_size;
      default: break;
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

void audio_update() {
  if (!device_id || SDL_GetAudioDeviceStatus(device_id) != SDL_AUDIO_PLAYING) return;

  uint32_t queued = SDL_GetQueuedAudioSize(device_id);
  uint32_t free_space = sbuf_size - queued;

  if (free_space == 0) return;

  uint32_t len = (count < free_space) ? count : free_space;
  if (len == 0) return;

  uint8_t *buffer = malloc(len);  // 临时缓冲区
  if (read_pos + len <= sbuf_size) {
    memcpy(buffer, sbuf + read_pos, len);
    read_pos += len;
  } else {
    uint32_t first_chunk = sbuf_size - read_pos;
    memcpy(buffer, sbuf + read_pos, first_chunk);
    memcpy(buffer + first_chunk, sbuf, len - first_chunk);
    read_pos = len - first_chunk;
  }

  if (read_pos >= sbuf_size) {
    read_pos -= sbuf_size;
  }

  count -= len;

  SDL_QueueAudio(device_id, buffer, len);
  free(buffer);
}