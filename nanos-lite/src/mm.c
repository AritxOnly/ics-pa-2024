#include <memory.h>
#include <proc.h>

static void *pf = NULL;
extern PCB *current;

void* new_page(size_t nr_page) {
  void *ret = pf;
  pf += nr_page * PGSIZE;
  
  return ret;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  assert(n % PGSIZE == 0 && "Allocating space can't be calculated...");
  void* page = new_page(n / PGSIZE);
  memset(page, 0, n);
  return page;
}
#endif

void free_page(void *p) {
  // panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  // intptr_t old_brk = current->max_brk;
  
  // if (brk <= old_brk) {
  //   return 0;
  // }

  // uintptr_t start = (old_brk + PGSIZE - 1) & ~(PGSIZE - 1);
  // uintptr_t end   = (brk     + PGSIZE - 1) & ~(PGSIZE - 1);

  // uintptr_t va = start;
  // for (; va < end; va += PGSIZE) {
  //   void *pa = pg_alloc(PGSIZE);
  //   map(&(current->as), (void *)va, pa, 0b111);
  // }

  // if (brk > current->max_brk) {
  //   current->max_brk = va;
  // }
  uintptr_t curbrk = current->max_brk;
  //printf("brk=%x, curbrk=%x\n",brk, curbrk);
  if (brk > curbrk) {
    while (brk > curbrk) {
      map(&(current->as), (char*)curbrk, pg_alloc(PGSIZE), 0b111);
      curbrk += PGSIZE;
      //printf("brk=%x, curbrk=%x\n",brk, curbrk);
    }
    current->max_brk = curbrk;
  }

  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
