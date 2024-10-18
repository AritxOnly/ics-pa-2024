#define BAD_EXIT_STATUS nemu_state.state == NEMU_ABORT || nemu_state.halt_ret != 0

#define IRINGBUF_SIZE 16

typedef struct {
  char inst[IRINGBUF_SIZE][256];
  int head;
} IRingBuf;

// parse elf
#include <elf.h>

typedef struct {
  Elf32_Addr st_value;  // 符号值（地址）
  char *st_name;
  Elf32_Word st_size; // 符号大小
  uint8_t st_info;  // 符号类型
} Symbol;