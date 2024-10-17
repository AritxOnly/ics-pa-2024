#define BAD_EXIT_STATUS nemu_state.state == NEMU_ABORT || nemu_state.halt_ret != 0

#define IRINGBUF_SIZE 16

typedef struct {
  char inst[IRINGBUF_SIZE][256];
  int head;
} IRingBuf;