#include <common.h>

void do_syscall(Context *c);

#define TIMER_LOG Log("IRQ TIMER recieved");

static Context* do_event(Event e, Context* c) {
  switch (e.event) {
    case EVENT_YIELD: return schedule(c); break;
    case EVENT_IRQ_TIMER: /* TIMER_LOG */ return schedule(c); break;
    case EVENT_SYSCALL: do_syscall(c); break;
    default: panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
