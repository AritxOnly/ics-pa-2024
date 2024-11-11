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

#include <isa.h>
#include "../local-include/reg.h"

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   */
  csr(MEPC) = epc;
  csr(MCAUSE) = NO;

  /* 	1.	将 MPIE 清零（第 7 位）。
	 *  2.	将当前的 MIE（第 3 位） 状态保存到 MPIE，相当于“备份”当前的中断状态。
	 *  3.	将 MIE 清零，以禁用机器模式的全局中断 
   */
   csr(MSTATUS) = csr(MSTATUS) & 0xffffff7ff; // MPIE：在进入异常或中断时保存当前的中断使能状态
   csr(MSTATUS) = (((csr(MSTATUS) >> 3) & 1) << 7) | csr(MSTATUS);
   csr(MSTATUS) = csr(MSTATUS) & 0xfffffff7;  // MIE：用于控制机器模式下的全局中断使能

  return csr(MTVEC);
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
