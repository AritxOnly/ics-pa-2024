#include <common.h>
#include "syscall.h"

void strace_info(uintptr_t *a) {
  Log("Strace raising syscall %d", a[0]);
#if defined (__riscv)
  printf("----------------------------------------\n");
  printf("Strace information\n");
  printf("----------------------------------------\n");
  printf("Raising syscall: %d\n", a[0]);
  printf("(For riscv32)Rigisters:\n");
  printf("a0: 0x%08x(%d)\n", a[0], a[0]);
  printf("a0: 0x%08x(%d)\n", a[1], a[1]);
  printf("a0: 0x%08x(%d)\n", a[2], a[2]);
  printf("----------------------------------------\n");
#else
  // panic("STRACE for your selected ISA is not implemented!");
#endif
}

size_t fs_write(int fd, const void *buf, int len);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  
#ifdef ENABLE_STRACE
  strace_info(a);
#endif

  switch (a[0]) {
    case SYS_yield: yield(); c->GPRx = 0; break;
    case SYS_exit: halt(a[1]); break;
    case SYS_write: c->GPRx = fs_write(a[0], (void *)a[1], a[2]); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
