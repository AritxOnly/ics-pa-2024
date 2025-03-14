#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

Context* __am_irq_handle(Context *c) {
  __am_get_cur_as(c);

  if (user_handler) {
    Event ev = {0};
    // printf("c->mcause = %d\n", c->mcause);

    /* 在navy-apps/libs/libos/syscall.h中有一个enum的定义 */
    if (c->mcause >= 0 && c->mcause < 20) {
      ev.event = EVENT_SYSCALL;
      goto goto_finished;
    }

    switch (c->mcause) {
      case -1: ev.event = EVENT_YIELD; break;
      case 0x80000007: ev.event = EVENT_IRQ_TIMER; goto goto_skip_mepc_incr; break;
      default: 
        printf("Unkown mcause code: 0x%8x(%d)\n", c->mcause); 
        ev.event = EVENT_ERROR; 
        break;
    }

  goto_finished:
    c->mepc += 4;

  goto_skip_mepc_incr:
    c = user_handler(ev, c);
    assert(c != NULL);
  }

  __am_switch(c);
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

#define MSTATUS_MMP  0x1800
#define MSTATUS_MPIE 0x80

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *context = (Context *)(kstack.end - sizeof(Context));

  memset(context, 0, sizeof(Context));

  uintptr_t sp = (uintptr_t)kstack.end & ~0xf;  // 16字节对齐，栈顶

  context->gpr[2]   = sp;
  context->mepc     = (uintptr_t)entry;
  context->mstatus  = MSTATUS_MMP | MSTATUS_MPIE;
  context->pdir     = NULL;
  context->gpr[10]  = (uintptr_t)arg;

  return context;
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
