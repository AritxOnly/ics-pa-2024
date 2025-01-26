#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

#define PTESIZE   8

#define VPN1_MASK 0xffc00000
#define VPN0_MASK 0x003ff000

#define V_MASK (1 << 0)  // valid
#define R_MASK (1 << 1)  // read
#define W_MASK (1 << 2)  // write
#define X_MASK (1 << 3)  // execute

void map(AddrSpace *as, void *va, void *pa, int prot) {
  uintptr_t pdir = (uintptr_t)(as->ptr) >> 12;    // 顶级页表的PPN

  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t vpn1 = (vaddr & VPN1_MASK) >> 22;  // 高10位下移到低位
  uintptr_t vpn0 = (vaddr & VPN0_MASK) >> 12;  // 中10位下移到低位

  uintptr_t paddr = (uintptr_t)pa;

  uintptr_t pde_addr = pdir * PGSIZE + vpn1 * PTESIZE;

  uintptr_t pte_addr;
  if (*(uintptr_t *)pde_addr == 0) {
    pte_addr = (uintptr_t)pgalloc_usr(PGSIZE);
    *(uintptr_t *)pte_addr = ((pde_addr & (VPN0_MASK | VPN1_MASK)) >> 2) | V_MASK;
  } else {
    uintptr_t pte_ppn = ((*(uintptr_t*)pde_addr) & 0xfffffc00) >> 10;
    pte_addr = pte_ppn * PGSIZE + (vpn0 >> 12) * PTESIZE; 
  }

  uintptr_t pte = ((paddr & VPN1_MASK) >> 2) | ((paddr & VPN0_MASK) >> 2) | 
                  X_MASK | W_MASK | R_MASK | V_MASK;
  
  *(uintptr_t *)pte_addr = pte;
}

#define MSTATUS_MMP  0x1800
#define MSTATUS_MPIE 0x80

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *context = (Context *)(kstack.end - sizeof(Context));

  memset(context, 0, sizeof(Context));

  // printf("[ucontext: %d] as->ptr = %p\n", __LINE__, as->ptr);

  context->mepc    = (uintptr_t)entry;
  context->mstatus = MSTATUS_MMP | MSTATUS_MPIE;  
  context->gpr[2]  = (uintptr_t)context;
  context->pdir    = as->ptr;

  return context;
}
