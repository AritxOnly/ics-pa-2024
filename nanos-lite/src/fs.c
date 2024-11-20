#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

typedef int off_t;

typedef struct {
  Finfo *file;
  off_t offset;
  bool opened;
} OpenFile;

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

#define MAX_OPEN_FILES 128
static OpenFile open_files[MAX_OPEN_FILES] = {
  [FD_STDIN] = { &file_table[0], 0, true },
  [FD_STDOUT] = { &file_table[1], 0, true },
  [FD_STDERR] = { &file_table[2], 0, true },
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
  open_files[fd].file = &file_table[fd];
  open_files[fd].offset = 0;
  open_files[fd].opened = true;
  return fd;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES || !open_files[fd].opened) {
    panic("Invalid 'fd' value %d", fd);
  }

  OpenFile *of = &open_files[fd];
  Finfo *f = of->file;
  size_t read_len = 0;
  size_t offset = f->disk_offset + of->offset;

  if (f->read) {
    read_len = f->read(buf, offset, len);
  } else {
    if (of->offset >= f->size) {
      return 0;
    }

    // 计算实际可读取的长度，防止越界
    size_t max_len = f->size - of->offset;
    if (len > max_len) {
      len = max_len;
    }

    if (len == 0) {
      return 0;
    }

    read_len = ramdisk_read(buf, offset, len);
  }
  of->offset += read_len;
  return read_len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= NR_FILES || !open_files[fd].opened) {
    panic("Invalid 'fd' value %d", fd);
  }

  OpenFile *of = &open_files[fd];
  Finfo *f = of->file;
  size_t write_len = 0;
  size_t offset = f->disk_offset + of->offset;

  if (f->write) {
    Log("reached here");
    write_len = f->write(buf, offset, len);
  } else {
    if (of->offset >= f->size) {
      return 0;
    }

    size_t max_len = f->size - of->offset;
    if (len > max_len) {
      len = max_len;
    }

    if (len == 0) {
      return 0;
    }

    write_len = ramdisk_write(buf, offset, len);
  }
  of->offset += write_len;
  return write_len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  if (fd < 0 || fd >= NR_FILES || !open_files[fd].opened) {
    panic("Invalid 'fd' value %d", fd);
  }

  OpenFile *of = &open_files[fd];
  Finfo *f = of->file;
  off_t new_offset;

  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset = of->offset + offset;
      break;
    case SEEK_END:
      new_offset = f->size + offset;
      break;
    default:
      panic("Invalid whence %d", whence);
  }

  if (new_offset < 0 || new_offset > f->size) {
    panic("Invalid offset after lseek");
  }

  of->offset = new_offset;
  return of->offset;
}

int fs_close(int fd) {
  return 0;
}