#include <common.h>
#include "syscall.h"
#include <proc.h>

void strace_info(uintptr_t *a) {
  Log("Strace raising syscall %d", a[0]);
#if defined (DETAILED_STRACE)
  printf("----------------------------------------\n");
  printf("Strace information\n");
  printf("----------------------------------------\n");
  printf("Raising syscall: %d\n", a[0]);
  printf("(For riscv32)Rigisters:\n");
  printf("a0: 0x%08x(%d)\n", a[0], a[0]);
  printf("a1: 0x%08x(%d)\n", a[1], a[1]);
  printf("a2: 0x%08x(%d)\n", a[2], a[2]);
  printf("----------------------------------------\n");
#else
  // panic("STRACE for your selected ISA is not implemented!");
#endif
}

struct timeval {
  long tv_sec;
  long tv_usec;
};

void naive_uload(PCB *pcb, const char *filename);

static char cur_bin[64] = ENTRY_BIN;

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
    case SYS_exit: 
      // halt(a[1]); 
      Log("ENTRY: %s, cur: %s", ENTRY_BIN, cur_bin);
      if (strcmp(ENTRY_BIN, cur_bin) == 0) {
        halt(0);
      } else {
        naive_uload(NULL, ENTRY_BIN);
      }
      c->GPRx = 0;
      break;
    case SYS_open: c->GPRx = fs_open((char *)a[1], a[2], a[3]); break;
    case SYS_read: c->GPRx = fs_read(a[1], (void *)a[2], a[3]); break;
    case SYS_write: c->GPRx = fs_write(a[1], (void *)a[2], a[3]); break;
    case SYS_lseek: c->GPRx = fs_lseek(a[1], a[2], a[3]); break;
    case SYS_close: c->GPRx = fs_close(a[1]); break;
    case SYS_gettimeofday:
      uint32_t clock = io_read(AM_TIMER_UPTIME).us;
      struct timeval *tv = (struct timeval *)a[1];
      if (tv == NULL) {
        c->GPRx = -1;
        break;
      }
      tv->tv_sec = clock / 1000000;
      tv->tv_usec = clock % 1000000;
      c->GPRx = 0;
      break;
    case SYS_brk: c->GPRx = 0; break;
    case SYS_execve:
      strncpy(cur_bin, (const char *)a[1], strlen((const char *)a[1]) + 1);
      // naive_uload(NULL, (const char *)a[1]);
      context_uload(current, cur_bin, (char *const *)a[2], (char *const *)a[3]);
      // c->GPRx = 0;
      switch_boot_pcb();
      yield();
      // #warning TODO
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
