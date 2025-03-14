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

// #define SIMPLE_SCHEDULE
// #define DIFFTEST_SCHEDULE

void init_proc() {
  char *const argv[] = {NULL};
  context_uload(&pcb[0], "/bin/menu", argv, (char *const *){NULL});
  // context_kload(&pcb[1], hello_fun, (void *)0x114514);
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here
  // naive_uload(NULL, ENTRY_BIN);
}

#if !defined (SIMPLE_SCHEDULE)
#if defined (DIFFTEST_SCHEDULE)
// Copied from https://github.com/yfxiang0112/njupa2023 for difftest
static int curr_idx=3;

Context* schedule(Context *prev) {
  current->cp = prev;
  while(1) {
    curr_idx = (curr_idx+1)%4;
    if (pcb[curr_idx].cp != 0) {
      current = &pcb[curr_idx];
      break;
    }
  }
  return current->cp;
}
#else
static int current_proc = -1;

Context* schedule(Context *prev) {
  current->cp = prev;

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
#endif
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
