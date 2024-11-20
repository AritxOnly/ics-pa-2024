#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
  bool opened;
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
  [FD_STDIN]  = {"stdin", 0, 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, 0,invalid_read, stdout_write},
  [FD_STDERR] = {"stderr", 0, 0, 0, invalid_read, stdout_write},
#include "files.h"
};

void init_fs() {
  // TODO: initialize the size of /dev/fb
}

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))
#define NR_PRESET 3

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

  Finfo *f = &file_table[fd];
  if (f->opened) {
    panic("fd %d is opened", fd);
  }

  if (!f->read) {
    f->read = ramdisk_read;
  }
  if (!f->write) {
    f->write = ramdisk_write;
  }

  f->opened = true;
  f->open_offset = 0;

  return fd;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }

  size_t read_len = 0;
  Finfo *f = &file_table[fd];
  read_len = (f->open_offset + len > f->size) ? (f->size - f->open_offset) : len;
  size_t offset = f->disk_offset + f->open_offset;

  if (f->read) {
    read_len = f->read(buf, offset, read_len);
  } else {
    panic("Unimplemented read function for fd: %d", fd);
  }

  if (fd >= NR_PRESET) {
    f->open_offset += read_len;
  }

  return read_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES) {
    panic("Invalid 'fd' value %d", fd);
  }

  size_t write_len = 0;
  Finfo *f = &file_table[fd];
  write_len = (f->open_offset + len > f->size) ? (f->size - f->open_offset) : len;
  size_t offset = f->disk_offset + f->open_offset;

  if (f->write) {
    write_len = f->write(buf, offset, write_len);
  } else {
    panic("Unimplemented write function for fd: %d", fd);
  }

  if (fd >= NR_PRESET) {
    f->open_offset += write_len;
  }

  return write_len;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  if (fd < 0 || fd >= NR_FILES ) {
    panic("Invalid 'fd' value %d", fd);
  }

  Finfo *f = &file_table[fd];
  switch(whence) {
    case SEEK_SET:
      if (offset > f->size || offset < 0) {
        panic("Out of memory at 0x%08x", f->disk_offset + offset);
      }
      f->open_offset = offset;
      break;
    case SEEK_CUR:
      if (offset + f->open_offset > f->size || offset + f->open_offset < 0) {
        panic("Out of memory at 0x%08x", f->disk_offset + f->open_offset + offset);
      }  
      f->open_offset += offset;
      break;
    case SEEK_END:
      f->open_offset = f->size + offset;
      break;
    default:
      panic("Should not reach here");
  }

  return f->open_offset;
}

int fs_close(int fd) {
  Finfo *f = &file_table[fd];
  f->opened = false;
  return 0;
}