#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t inner_offset;
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

int fs_open(const char *pathname, int flags, int mode) {
  /* ignore flags and mode */
  int fd = -1;
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      fd = i;
      break;
    }
  }
  if (fd == -1) {
    panic("File %s not found", pathname);
  }
  return fd;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }

  Finfo *f = &file_table[fd];
  if (f->read) {
    size_t read_len = f->read(buf, f->inner_offset + f->disk_offset, len);
    f->inner_offset += read_len;
    return read_len;
  }
  Log("Reached here");
  return -1;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }
  
  Finfo *f = &file_table[fd];
  if (f->write) {
    size_t write_len = f->write(buf, f->inner_offset, len);
    f->inner_offset += write_len;
    return write_len;
  }
  return -1;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }

  Finfo *f = &file_table[fd];
  switch (whence) {
    case SEEK_SET: f->inner_offset = offset; break;
    case SEEK_CUR: f->inner_offset += offset; break;
    case SEEK_END: 
      f->inner_offset = (offset > 0) ? f->inner_offset : f->size + offset; 
      break;
    default: panic("Invalid whence %d", whence);
  }
  return f->inner_offset;
}

int fs_close(int fd) {
  return 0;
}