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
  const char *ch_buf = buf;
  for (size_t i = 0; i < len; i++) {
    putch(ch_buf[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  AM_INPUT_KEYBRD_T kv = io_read(AM_INPUT_KEYBRD);
  if (kv.keycode != AM_KEY_NONE) {
    return snprintf(buf, len, "k%c %s", (kv.keydown ? 'd' : 'u'), keyname[kv.keycode]);
  }
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
  if (config.present) {
    return snprintf(buf, len, "%d %d", config.width, config.height);
  }
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
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

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
