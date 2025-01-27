#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

#define SIMPLE_SCHEDULE

void init_proc() {
  char *const argv[] = {NULL};
  context_uload(&pcb[0], ENTRY_BIN, argv, (char *const *){NULL});
#if defined (SIMPLE_SCHEDULE)
  context_kload(&pcb[1], hello_fun, (void *)0x114514);
#endif
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, ENTRY_BIN);
}

#if !defined (SIMPLE_SCHEDULE)
static int current_proc = -1;

Context* schedule(Context *prev) {
  if (prev && current_proc != -1) {
    pcb[current_proc].cp = prev;  // 保存现场到该PCB
  }

  // 切换到下一个非空的PCB进程
  do {
    current_proc = (current_proc + 1) % MAX_NR_PROC;
    current = &pcb[current_proc];   // 更新全局指针
    // Log("current_proc = %d, current = %p, current->cp = %p", current_proc, current, current->cp);
  } while (!current->cp);

  // Log("mepc: %p", current->cp->mepc);

  // 返回下一个进程的cp，进入新的上下文
  return current->cp;
}
#else
Context* schedule(Context *prev) {
  // save the context pointer
  current->cp = prev;

  // switch between pcb[0] and pcb[1]
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

  // then return the new context
  return current->cp;
}
#endif
