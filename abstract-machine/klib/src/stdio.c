#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#define PRESAVE_BUFFER 256

void int_parse(char* dest, int v) {
  char tmp_str[PRESAVE_BUFFER];
  size_t i = 0;
  int is_negative = 0;
  if (v < 0) {
    is_negative = 1;
    v = -v;
  }
  do {
    int digit = v % 10;
    tmp_str[i++] = digit + '0';
    v /= 10;
  } while (v != 0);
  if (is_negative) {
    tmp_str[i++] = '-';
  }
  while (i--) {
    *(dest++) = tmp_str[i];
  }
  *dest = '\0';
}

void int_hex_parse(char* dest, uintptr_t v) {
  char tmp_str[PRESAVE_BUFFER];
  size_t i = 0;
  do {
    unsigned int rem = v % 16;
    if (rem < 10) {
      tmp_str[i++] = rem + '0';
    } else {
      tmp_str[i++] = (rem - 10) + 'a';
    }
    v /= 16;
  } while (v != 0);
  while (i--) {
    *(dest++) = tmp_str[i];
  }
  *dest = '\0';
}

void pre_space(char* dest, char space, int len, int buf_sz) {
  int curr = strlen(dest);
  int after = (curr >= len) ? curr : len;
  int shift = after - curr;
  if (!shift)  return;
  assert(curr + shift < buf_sz);
  // printf("curr = %d, shift = %d\n", curr, shift);
  for (int i = curr; i >= 0; i--) {
    dest[i + shift] = dest[i];
  }
  for (int i = 0; i < shift; i++) {
    dest[i] = space;
  }
}

static inline int isdigit(unsigned char ch) {
  return (ch >= '0' && ch <= '9');
}

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsprintf(char *out, const char *fmt, va_list args) {
  // 使用一个较大的值，表示无限制长度
  return vsnprintf(out, SIZE_MAX, fmt, args);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(out, SIZE_MAX, fmt, args);
  va_end(args);
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(out, n, fmt, args);
  va_end(args);
  return len;
}

#define BUFFER_MAX 8192

int printf(const char *fmt, ...) {
  char buffer[BUFFER_MAX];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buffer, BUFFER_MAX, fmt, args);
  va_end(args);
  for (int i = 0; i < len; i++) {
    putch(buffer[i]);
  }
  return len;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list args) {
  char *str = out;
  const char *p = fmt;
  size_t remaining = n;
  char ch;

  while ((ch = *p++) != '\0') {
    if (ch != '%') {
      if (remaining > 1) {
        *str++ = ch;
        remaining--;
      } else if (remaining == 1) {
        // 缓冲区只剩下空间放 '\0'，不再写入字符
        remaining--;
      }
      continue;
    }

    ch = *p++;
    if (ch == '\0') {
      // 格式字符串以 '%' 结束，输出 '%'
      if (remaining > 1) {
        *str++ = '%';
        remaining--;
      }
      break;
    }

    char space = ' ';
    int cnt = 0;

    // 处理可能的填充字符和宽度
    if (ch == '0') {
      space = '0';
      ch = *p++;
    }

    if (isdigit((unsigned char)ch)) {
      cnt = 0;
      while (isdigit((unsigned char)ch)) {
        cnt = cnt * 10 + (ch - '0');
        ch = *p++;
      }
    }

    switch (ch) {
      case 'd': {
        int val = va_arg(args, int);
        char buf[PRESAVE_BUFFER];
        int_parse(buf, val);

        if (val < 0 && space == '0') {
          pre_space(buf + 1, space, cnt - 1, PRESAVE_BUFFER - 1);
        } else {
          pre_space(buf, space, cnt, PRESAVE_BUFFER);
        }

        size_t len = strlen(buf);
        size_t copy_len = (len < remaining - 1) ? len : remaining - 1;
        strncpy(str, buf, copy_len);
        str += copy_len;
        remaining -= copy_len;
        break;
      }

      case 's': {
        printf("reached here");
        char *val = va_arg(args, char *);
        if (val == NULL) val = "(null)"; // 防止 val 为 NULL
        size_t len = strlen(val);
        size_t copy_len = (len < remaining - 1) ? len : remaining - 1;
        strncpy(str, val, copy_len);
        printf("reached here2");
        str += copy_len;
        remaining -= copy_len;
        break;
      }

      case 'x': {
        uintptr_t val = va_arg(args, uintptr_t);
        char buf[PRESAVE_BUFFER];
        int_hex_parse(buf, val);
        pre_space(buf, space, cnt, PRESAVE_BUFFER);

        size_t len = strlen(buf);
        size_t copy_len = (len < remaining - 1) ? len : remaining - 1;
        strncpy(str, buf, copy_len);
        str += copy_len;
        remaining -= copy_len;
        break;
      }

      case 'p': {
        void *ptr = va_arg(args, void *);
        uintptr_t val = (uintptr_t)ptr;
        char buf[PRESAVE_BUFFER];
        strcpy(buf, "0x");
        int_hex_parse(buf + 2, val);
        pre_space(buf, space, cnt, PRESAVE_BUFFER);

        size_t len = strlen(buf);
        size_t copy_len = (len < remaining - 1) ? len : remaining - 1;
        strncpy(str, buf, copy_len);
        str += copy_len;
        remaining -= copy_len;
        break;
      }

      default:
        if (remaining > 1) {
          *str++ = '%';
          remaining--;
        }
        if (remaining > 1) {
          *str++ = ch;
          remaining--;
        }
        break;
    }
  }

  if (n > 0) {
    *str = '\0';
  }

  return str - out;
}

#endif