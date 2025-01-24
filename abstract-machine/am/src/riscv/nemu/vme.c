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

#define PTESIZE   4      // PTE大小

#define VPN1_MASK 0xffc00000   // 高10位
#define VPN0_MASK 0x003ff000   // 中10位
#define PPN1_MASK 0xffc00000   // 物理地址高10位
#define PPN0_MASK 0x003ff000   // 物理地址中10位

#define V_MASK (1 << 0)  // valid
#define R_MASK (1 << 1)  // read
#define W_MASK (1 << 2)  // write
#define X_MASK (1 << 3)  // execute

void map(AddrSpace *as, void *va, void *pa, int prot) {
  uintptr_t pdir_ppn = (uintptr_t)(as->ptr) >> 12;    // 顶级页表的PPN
  uintptr_t pdir_base = pdir_ppn * PGSIZE;            // 顶级页表在物理内存中的起始地址

  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t vpn1 = (vaddr & VPN1_MASK) >> 22;  // 高10位下移到低位
  uintptr_t vpn0 = (vaddr & VPN0_MASK) >> 12;  // 中10位下移到低位

  uintptr_t paddr = (uintptr_t)pa;
  uintptr_t ppn1 = (paddr & PPN1_MASK) >> 2;   // 原代码把 ppn1>>2 放到PTE高位
  uintptr_t ppn0 = (paddr & PPN0_MASK) >> 2;   // 同理

  uintptr_t pde_addr = pdir_base + vpn1 * PTESIZE;

  uintptr_t *pde_ptr = (uintptr_t *)pde_addr; 
  uintptr_t pde_val = *pde_ptr;

  uintptr_t ptable_base;
  if (pde_val == 0) {
    // 分配一页物理内存用于二级页表
    ptable_base = (uintptr_t)pgalloc_usr(PGSIZE);
    // 清零(保证新的页表中没有陈旧数据)
    memset((void*)ptable_base, 0, PGSIZE);

    // 构造 PDE
    uintptr_t pde_new = ((ptable_base & 0xfffff000) >> 2) | V_MASK;

    // 写回 PDE
    *pde_ptr = pde_new;
  } else {
    // 已经存在二级页表 => 取出其 PPN, 得到二级页表基地址(physical)
    uintptr_t pte_ppn = ((pde_val & 0xfffffc00) >> 10);
    ptable_base = pte_ppn * PGSIZE;
  }

  // 计算二级页表项(PTE)在物理地址空间中的位置
  uintptr_t pte_addr = ptable_base + vpn0 * PTESIZE;

  // 如果 prot == 0 => 表示取消映射 => 写0即可
  if (prot == 0) {
    *(uintptr_t*)pte_addr = 0;
    return;
  }

  // 否则，构造新的PTE来映射 [va => pa]
  uintptr_t flags = V_MASK | R_MASK | W_MASK | X_MASK;
  // 拼出最终的 pte
  uintptr_t pte_val = (ppn1 | ppn0) | flags;

  // 写回二级页表项
  *(uintptr_t*)pte_addr = pte_val;
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *context = (Context *)(kstack.end - sizeof(Context));

  memset(context, 0, sizeof(Context));

  context->mepc    = (uintptr_t)entry;
  context->mstatus = 0x1800;
  context->gpr[2]  = (uintptr_t)context;

  return context;
}
