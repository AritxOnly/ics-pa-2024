/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __RISCV_REG_H__
#define __RISCV_REG_H__

#include <common.h>

static inline int check_reg_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < MUXDEF(CONFIG_RVE, 16, 32)));
  return idx;
}

#define gpr(idx) (cpu.gpr[check_reg_idx(idx)])

static inline const char* reg_name(int idx) {
  extern const char* regs[];
  return regs[check_reg_idx(idx)];
}

/* CSRs */

/* CSRs寄存器映射 */
#define MEPC 0x341
#define MSTATUS 0x300
#define MCAUSE 0x342
#define MTVEC 0x305
#define MSCRATCH 0x340
#define SATP 0x180

static inline int check_csr_idx(int idx) {
  IFDEF(CONFIG_RT_CHECK, assert(idx >= 0 && idx < 4096));
  return idx;
}

static inline int csr_hash(int idx) {
  switch (check_csr_idx(idx)) {
    case MEPC: return 0; break;
    case MSTATUS: return 1; break;
    case MCAUSE: return 2; break;
    case MTVEC: return 3; break;
    case MSCRATCH: return 4; break;
    case SATP: return 5; break;
    default: Assert(0, "Unimplemented CSRs");
  }
  return -1;
}

#define csr(idx) (cpu.csr[csr_hash(idx)])

static inline const char* csr_name(int idx) {
  extern const char* csrs[];
  return csrs[csr_hash(idx)];
}

#endif
