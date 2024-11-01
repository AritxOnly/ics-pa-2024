#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

void int_parse(char* dest, int v) {
  char tmp_str[20];
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

void int_hex_parse(char* dest, int v) {
  char tmp_str[20];
  size_t i = 0;
  do {
    int rem = v % 16;
    if (rem < 10) {
      tmp_str[i++] = rem + '0';
    } else {
      tmp_str[i++] = (rem - 10) + 'a';
    }
    v /= 16;
  } while (v != 0);
  tmp_str[i++] = 'x';
  tmp_str[i++] = '0';
  while (i--) {
    *(dest++) = tmp_str[i];
  }
  *dest = '\0';
}

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int vsprintf(char *out, const char *fmt, va_list args) {
  char *str = out;
  const char *p = fmt;
  char ch;
  while ((ch = *p++)) {
    if (ch != '%') {
      *str++ = ch;
      continue;
    }
    ch = *p++;
    switch (ch) {
      case 'd': {
        int val = va_arg(args, int);
        char buf[20];
        int_parse(buf, val);
        strcpy(str, buf);
        str += strlen(buf);
        break;
      }
      case 's': {
        char *val = va_arg(args, char *);
        strcpy(str, val);
        str += strlen(val);
        break;
      }
      case 'x': {
        int val = va_arg(args, int);
        char buf[20];
        int_hex_parse(buf, val);
        strcpy(str, buf);
        str += strlen(buf);
        break;
      }
      case '%': {
        *str++ = '%';
        break;
      }
      default:
        *str++ = '%';
        *str++ = ch;
        break;
    }
  }
  *str = '\0';
  return str - out;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(out, fmt, args);
  va_end(args);
  return len;
}

#define BUFFER_MAX 8192

int printf(const char *fmt, ...) {
  char buffer[BUFFER_MAX];
  va_list args;
  va_start(args, fmt);
  int len = vsprintf(buffer, fmt, args);
  va_end(args);
  for (int i = 0; i < len; i++) {
    putch(buffer[i]);
  }
  return len;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(out, n, fmt, args);
  va_end(args);
  return len;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list args) {
  int len = vsprintf(out, fmt, args);
  if (len >= n) {
    out[n - 1] = '\0';
    len = n - 1;
  }
  return len;
}

#endif