#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  assert(s != NULL);
  size_t cnt = 0;
  while (s[cnt] != '\0') {
    cnt++;
    if (cnt == 0) {
      return cnt - 1;
    }
  }
  return cnt;
}

char *strcpy(char *dst, const char *src) {
  assert(dst != NULL && src != NULL);
  char *ptr_d = dst;
  const char *ptr_s = src;
  while (*ptr_s != '\0') {
    *ptr_d = *ptr_s;
    ptr_d++;
    ptr_s++;
  }
  *ptr_d = *ptr_s;
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  assert(dst != NULL && src != NULL);
  for (int i = 0; i < n; i++) {
    dst[i] = src[i];
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  assert(dst != NULL && src != NULL);
  char *ptr_d = dst;
  while (*ptr_d != '\0') {
    ptr_d++;
  }
  const char *ptr_s = src;
  while (*ptr_s != '\0') {
    *ptr_d = *ptr_s;
    ptr_d++;
    ptr_s++;
  }
  *ptr_d = *ptr_s;
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  assert(s1 != NULL && s2 != NULL);
  while (*s1 == *s2) {
    if (*s1 == '\0') {
      return 0;
    }
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  assert(s1 != NULL && s2 != NULL);
  for (int i = 0; i < n; i++) {
    if (s1[i] != s2[i]) {
      return s1[i] - s2[i];
    } else if (s1[i] == '\0') {
      return 0;
    }
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  // memset是将每个字节都设置为c
  uint8_t *ptr = s;
  for (int i = 0; i < n; i++) {
    *(ptr + i) = (uint8_t)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  // 如果目标区域和源区域有重叠的话，memmove() 
  // 能够保证源串在被覆盖之前将重叠区域的字节拷贝到目标区域中，复制后源区域的内容会被更改
  uint8_t *ptr_d = dst;
  const uint8_t *ptr_s = src;
  if (ptr_d > ptr_s) {  // src在dst前面，从后往前拷贝避免重叠
    for (int i = n - 1; i >= 0; i--) {
      *(ptr_d + i) = *(ptr_s + i);
    }
  } else if (ptr_d < ptr_s) {
    for (int i = 0; i < n; i++) {
      *(ptr_d + i) = *(ptr_s + i);
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  uint8_t *ptr_o = out;
  const uint8_t *ptr_i = in;
  for (int i = 0; i < n; i++) {
    *(ptr_o + i) = *(ptr_i + i);
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *ptr1 = s1, *ptr2 = s2; 
  for (int i = 0; i < n; i++) {
    if (*(ptr1 + i) != *(ptr2 + i)) {
      return *(ptr1 + i) - *(ptr2 + i);
    }
  }
  return 0;
}

#endif
