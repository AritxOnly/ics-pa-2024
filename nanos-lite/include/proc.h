#ifndef __PROC_H__
#define __PROC_H__

#include <common.h>
#include <memory.h>

#define STACK_SIZE (8 * PGSIZE)

typedef union {
  uint8_t stack[STACK_SIZE] PG_ALIGN;
  struct {
    Context *cp;
    AddrSpace as;
    // we do not free memory, so use `max_brk' to determine when to call _map()
    uintptr_t max_brk;
  };
} PCB;

extern PCB *current;

extern Context *kcontext(Area kstack, void (*entry)(void *), void *arg);

void switch_boot_pcb();

void naive_uload(PCB *, const char *);
void context_uload(PCB *pcb, const char *pathname, char *const argv[], char *const envp[]);
void context_kload(PCB *pcb, void (*entry)(void *), void *arg);

#endif
