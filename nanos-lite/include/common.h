#ifndef __COMMON_H__
#define __COMMON_H__

/* Uncomment these macros to enable corresponding functionality. */
#define HAS_CTE
//#define HAS_VME
//#define MULTIPROGRAM
//#define TIME_SHARING

#define ENABLE_STRACE
#if defined (ENABLE_STRACE)
// #define DETAILED_STRACE
#endif

#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <debug.h>

typedef int off_t;

/* filesystem APIs */
int fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void *buf, size_t len);
size_t fs_write(int fd, const void *buf, size_t len);
off_t fs_lseek(int fd, off_t offset, int whence);
int fs_close(int fd);

/* device APIs */
size_t serial_write(const void *buf, size_t offset, size_t len);

#endif
