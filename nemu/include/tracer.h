#define IRINGBUF_SIZE 16

typedef struct {
  char inst[IRINGBUF_SIZE][256];
  int head;
} IRingBuf;