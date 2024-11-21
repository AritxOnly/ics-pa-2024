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
    Log("write %08x, %s, len %d", (intptr_t)buf, keyname[kv.keycode], len);
    Log("k%c %s", (kv.keydown ? 'd' : 'u'), keyname[kv.keycode]);
    size_t ret = snprintf(buf, len, "k%c %s", (kv.keydown ? 'd' : 'u'), keyname[kv.keycode]);
    return ret;
  }
  return 0;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
