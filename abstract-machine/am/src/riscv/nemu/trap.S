#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f( 28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

/* Context布局调整 */
#define CONTEXT_SIZE  ((NR_REGS + 5) * XLEN)  /* 新增np字段 */
#define OFFSET_SP      ( 2 * XLEN)            /* x2(sp)的位置 */
#define OFFSET_CAUSE   ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS  ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC     ((NR_REGS + 2) * XLEN)
#define OFFSET_NP      ((NR_REGS + 3) * XLEN) /* np字段偏移 */

.align 3
.globl __am_asm_trap
__am_asm_trap:
  /* Step 1: 原子交换sp和mscratch */
  csrrw sp, mscratch, sp  /* (1) 交换后sp=原mscratch, mscratch=原sp */
  
  /* Step 2: 判断原特权级 */
  bnez sp, from_user      /* (2) 原mscratch非0: 来自用户态 */
  
  /* 来自内核态: 恢复原sp */
  csrr sp, mscratch       /* (3) 原mscratch=0, 恢复原sp */
  j save_context

from_user:
  /* 此时sp已指向内核栈, 原用户栈指针在mscratch中 */
  /* 无需额外操作 */

save_context:
  /* 预留上下文空间 */
  addi sp, sp, -CONTEXT_SIZE

  /* Step 3: 保存原sp到Context(包括用户态或内核态的sp) */
  csrr t0, mscratch       /* 获取进入trap前的sp值 */
  STORE t0, OFFSET_SP(sp) /* 保存到Context的sp字段 */

  /* Step 4: 保存np字段 */
  li t0, 0                /* 默认内核态 */
  csrr t1, mscratch       /* 原mscratch值 */
  beqz t1, 1f
  li t0, 1                /* 用户态标记 */
1:
  STORE t0, OFFSET_NP(sp) /* 保存特权级标记 */

  /* Step 5: 强制mscratch为0(支持嵌套中断) */
  csrw mscratch, zero

  /* Step 6: 保存其他寄存器 */
  MAP(REGS, PUSH)

  /* 保存CSR寄存器 */
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc
  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  /* Step 7: 设置MPRV位(用于difftest) */
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  /* Step 8: 调用中断处理函数 */
  mv a0, sp
  jal __am_irq_handle

  /* Step 9: 切换到新上下文 */
  mv sp, a0

  /* 恢复CSR寄存器 */
  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  /* Step 10: 恢复通用寄存器 */
  MAP(REGS, POP)

  /* Step 11: 恢复sp和特权级 */
  LOAD t0, OFFSET_NP(sp)  /* 获取目标特权级 */
  LOAD sp, OFFSET_SP(sp)  /* 恢复目标sp */

  /* Step 12: 更新mscratch */
  beqz t0, 2f             /* 目标为内核态 */
  csrw mscratch, sp       /* 用户态: 保存内核栈指针到mscratch */
  j 3f
2: 
  csrw mscratch, zero     /* 内核态: mscratch置0 */
3:
  mret