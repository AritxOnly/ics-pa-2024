#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t stdout_write(const void *buf, size_t offset, size_t len) {
  const char *ch_buf = buf;
  for (size_t i = 0; i < len; i++) {
    putch(ch_buf[i]);
  }
  return len;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, stdout_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, stdout_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }
  
  Finfo *f = &file_table[fd];
  if (f->write) {
    return f->write(buf, 0, len);
  } else {
    return -1;
  }
}