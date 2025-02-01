#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  // yield();

  const char *ch_buf = buf;
  for (size_t i = 0; i < len; i++) {
    putch(ch_buf[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  // yield();

  AM_INPUT_KEYBRD_T kv = io_read(AM_INPUT_KEYBRD);
  if (kv.keycode != AM_KEY_NONE) {
    return snprintf(buf, len, "k%c %s", (kv.keydown ? 'd' : 'u'), keyname[kv.keycode]);
  }
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
  if (config.present) {
    return snprintf(buf, len, "WIDTH:%d\nHEIGHT:%d", config.width, config.height);
  }
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  // yield();
  
  if (!buf) {
    AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
    int w = config.width, h = config.height;
    io_write(AM_GPU_FBDRAW, 0, 0, NULL, w, h, true);
    return 0;
  }

  size_t pixel_offset = offset / sizeof(uint32_t);
  size_t pixels_to_write = len / sizeof(uint32_t);
  const uint32_t *pixels = (const uint32_t *)buf;

  AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
  int w = config.width;

  size_t x = pixel_offset % w;
  size_t y = pixel_offset / w;

  size_t remaining_pixels = pixels_to_write;
  size_t write_len = 0;

  while (remaining_pixels > 0) {
    size_t pixels_in_line = w - x;
    if (pixels_in_line > remaining_pixels) {
      pixels_in_line = remaining_pixels;
    }

    io_write(AM_GPU_FBDRAW, x, y, (void *)pixels, pixels_in_line, 1, false);

    pixels += pixels_in_line;
    remaining_pixels -= pixels_in_line;
    write_len += pixels_in_line;

    x = 0;
    y += 1;
  }

  io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true);

  return write_len * sizeof(uint32_t);
}

size_t sb_write(const void *buf, size_t offset, size_t len) {
  if (!buf) {
    return 0;
  }

  size_t written = 0;
  while (written < len) {
    AM_AUDIO_STATUS_T status = io_read(AM_AUDIO_STATUS);
    AM_AUDIO_CONFIG_T config = io_read(AM_AUDIO_CONFIG);

    uint32_t total_bufsize   = config.bufsize;
    uint32_t used_bufsize    = status.count;
    uint32_t remain_bufsize  = total_bufsize - used_bufsize;
    
    if (remain_bufsize == 0) {
      yield();  // 让出CPU
      continue; 
    }

    size_t left_to_write = len - written;
    size_t write_now     = (remain_bufsize >= left_to_write ? left_to_write
                                                            : remain_bufsize);

    Area audio_buf = {
      .start = (void*)((uintptr_t)buf + written),
      .end   = (void*)((uintptr_t)buf + written + write_now)
    };

    io_write(AM_AUDIO_PLAY, audio_buf);

    written += write_now;
  }

  return written;
}

size_t sbctl_write(const void *buf, size_t offset, size_t len) {
  if (len < 3 * sizeof(int)) {
    panic("len: %d of sbctl write should not be <12!", len);
  }

  int *info    = (int *)buf;

  int freq     = info[0];
  int channels = info[1];
  int samples  = info[2];

  io_write(AM_AUDIO_CTRL, freq, channels, samples);

  return 3 * sizeof(int);
}

size_t sbctl_read(void *buf, size_t offset, size_t len) {
  if (len < sizeof(int)) {
    panic("len: %d of sbctl write should not be <4!", len);
  }

  AM_AUDIO_CONFIG_T config = io_read(AM_AUDIO_CONFIG);
  AM_AUDIO_STATUS_T status = io_read(AM_AUDIO_STATUS);
  
  uint32_t total_bufsize   = config.bufsize;
  uint32_t used_bufsize    = status.count;
  uint32_t remain_bufsize  = total_bufsize - used_bufsize;

  int *info = (int *)buf;
  info[0]   = remain_bufsize;

  return sizeof(int);
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
