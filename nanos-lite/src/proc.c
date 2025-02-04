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
// #define MULTI_SCHEDULE

void init_proc() {
#if defined (MULTI_SCHEDULE)
  char *const argv[] = { "/bin/pal", "--skip", NULL };
  context_uload(&pcb[0], "/bin/hello", (char *const *){ NULL }, (char *const *){ NULL });
  context_uload(&pcb[1], "/bin/menu", (char *const *){ NULL }, (char *const *){ NULL });
  context_uload(&pcb[2], "/bin/pal", argv, (char *const *){ NULL });
  context_uload(&pcb[3], "/bin/nterm", (char *const *){ NULL }, (char *const *){ NULL });
#else
  context_uload(&pcb[0], "/bin/hello", (char *const *){ NULL }, (char *const *){ NULL });
  context_uload(&pcb[1], "/bin/pal", (char *const *){ NULL }, (char *const *){ NULL });
  // context_kload(&pcb[2], hello_fun, (void *)0x114514);
#endif
  // context_kload(&pcb[3], hello_fun, (void *)0x114514);
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
#if defined (MULTI_SCHEDULE)
static int curr_idx = 1; 
char ev_buf[32];
static uint32_t cycle = 0;

#define NUM_CYCLE 300

Context* schedule(Context* prev) {
  current->cp = prev;

  Log("cycle = %d, curr_idx = %d", cycle, curr_idx);
  
  events_read(ev_buf, 0, 31);
  Log("ev_buf == %s", ev_buf);

  if (strcmp(ev_buf, "ku F1") == 0) curr_idx = 1;
  if (strcmp(ev_buf, "ku F2") == 0) curr_idx = 2;
  if (strcmp(ev_buf, "ku F3") == 0) curr_idx = 3;

  curr_idx = (cycle == 0) ? 0 : curr_idx;

  cycle++;
  cycle %= NUM_CYCLE;

  current = &pcb[curr_idx];

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
