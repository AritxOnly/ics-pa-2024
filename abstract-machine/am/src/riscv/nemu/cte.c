#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    // printf("c->mcause = %d\n", c->mcause);

    /* 在navy-apps/libs/libos/syscall.h中有一个enum的定义 */
    if (c->mcause >= 0 && c->mcause < 20) {
      ev.event = EVENT_SYSCALL;
    } else if (c->mcause == -1) {
      ev.event = EVENT_YIELD;
    } else {
      printf("Unkown mcause code: 0x%8x(%d)\n", c->mcause);
      ev.event = EVENT_ERROR; 
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
