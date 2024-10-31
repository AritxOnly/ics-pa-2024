#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

void int_parse(char* dest, int v) {
  char tmp_str[20];
  size_t i = 0;
  if (v < 0) {
    tmp_str[i++] = '-';
    v = -v;
  }
  while (v != 0) {
    int digit = v % 10;
    tmp_str[i++] = digit + '0';
    v /= 10;
  }
  while (i--) {
    *(dest++) = tmp_str[i];
  }
  *dest = '\0';
}

void int_hex_parse(char* dest, int v) {
  char tmp_str[20];
  size_t i = 0;
  if (v < 0) {
    tmp_str[i++] = '-';
    v = -v;
  }
  while (v != 0) {
    int rem = v % 16;
    if (rem >= 0 && rem <= 9) {
      tmp_str[i++] = rem + '0';
    } else {
      tmp_str[i++] = (rem - 10) + 'A';
    }
    v /= 16;
  }
  while (i--) {
    *dest = tmp_str[i];
    dest++;
  }
  *dest = '\0';
}

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int sprintf(char *out, const char *fmt, ...);

#define BUFFER_MAX 256

int printf(const char *fmt, ...) {
  char chars[BUFFER_MAX] = { 0 };  // BUFFER_SIZE = 256 
  va_list args;
  va_start(args, fmt);
  int len = sprintf(chars, fmt, args);
  va_end(args);
  for (int i = 0; i < len && chars[i] != '\0'; i++) {
    putch(chars[i]);
  }
  return len;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);  // 初始化args，其中va_start二参是最后一个确定参数
  *out = '\0'; // 初始化
  char parsed[20] = {0};
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      memset(parsed, 0, sizeof(parsed));
      switch (*fmt) {
        case 'd': {
          int val = va_arg(args, int);
          int_parse(parsed, val);
          strcat(out, parsed);
          break;
        }
        case 's': {
          char* str = va_arg(args, char*);
          strcat(out, str);
          break;
        }
        case 'x': {
          int val = va_arg(args, int);
          int_hex_parse(parsed, val);
          strcat(out, parsed);
          break;
        }
        default:
          return -1;
      }
    } else {
      char append[] = {*fmt, '\0'};
      strcat(out, append);
    }
    fmt++;
  }
  va_end(args);
  return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
