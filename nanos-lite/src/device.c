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
  AM_GPU_CONFIG_T config = io_read(AM_GPU_CONFIG);
  int w = config.width, h = config.height;
  if (!buf) {
    io_write(AM_GPU_FBDRAW, 0, 0, NULL, w, h, true);
    return 0;
  } 
  int write_len = 0;
  while (offset != len) {
    int x = offset % w, y = offset / w;
    io_write(AM_GPU_FBDRAW, x, y, (void *)buf, 1, 2, true);
    offset++;
    write_len++;
    buf++;
  }
  return write_len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
