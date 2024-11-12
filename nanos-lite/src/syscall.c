#include <common.h>
#include "syscall.h"

void strace_info(uintptr_t *a) {
#if defined (__riscv)
  Log("----------------------------------------\n");
  Log("Strace information\n");
  Log("----------------------------------------\n");
  Log("Raising syscall: %ld\n", a[0]);
  Log("(For riscv32)Rigisters:\n");
  Log("a0: 0x%8lx(%ld)\n", a[0], a[0]);
  Log("a0: 0x%8lx(%ld)\n", a[1], a[1]);
  Log("a0: 0x%8lx(%ld)\n", a[2], a[2]);
  Log("----------------------------------------\n");
#else
  panic("STRACE for your selected ISA is not implemented!");
#endif
}

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
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
