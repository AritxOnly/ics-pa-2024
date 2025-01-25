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

#include "../local-include/reg.h"
#include <isa.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <memory/host.h>

int isa_mmu_check(vaddr_t vaddr, int len, int type) {
  uint32_t satp_val = csr(SATP);
  Log("satp: 0x%x", satp_val);
  if (vaddr >= 0x80000000) return MMU_DIRECT;
  // uint32_t mode = satp_val & 0xf;
  // if (mode == 1) {
  //   return MMU_TRANSLATE;
  // }
  if (satp_val == 0) {
    return MMU_DIRECT;
  }
  return MMU_TRANSLATE; 
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // 拆分虚拟地址
  paddr_t vpn1 = vaddr & 0xffc00000;  // 高10位
  paddr_t vpn0 = vaddr & 0x003ff000;  // 中10位
  paddr_t voff = vaddr & 0x00000fff;  // 低12位 (页内偏移)

  // 从 satp 中获取根页表物理页号(PPN)
  paddr_t satp_val = csr(SATP);          // 读寄存器
  paddr_t satp_ppn = satp_val & 0x003fffff;

  // 计算一级页表项地址
  paddr_t pte_addr = satp_ppn * 4096 + ((vpn1 >> 22) * 4);

  // 4. 读取一级页表项 (PTE1)
  word_t pte = host_read(guest_to_host(pte_addr), 4);

  // 检查一级页表项有效位(V=1)；若无效则抛出异常
  if (!(pte & 1)) {
    panic("invalid PTE: vaddr=0x%x, pte_addr=0x%x, pte=0x%x\n",
          vaddr, pte_addr, pte);
  }

  paddr_t paddr;

  // 判断是否是叶子节点
  if ((pte & 0x2) == 0 && (pte & 0x4) == 0 && (pte & 0x8) == 0) {
    paddr_t pte0_addr = ((pte & 0xfffffc00) >> 10) * 4096
                      + ((vpn0 >> 12) * 4);

    // 读取二级页表项 (PTE2)
    pte = host_read(guest_to_host(pte0_addr), 4);
    if (!(pte & 1)) {
      panic("invalid PTE (2nd-level): vaddr=0x%x, pte_addr=0x%x, pte=0x%x\n",
            vaddr, pte0_addr, pte);
    }

    // 从二级PTE中取出实际的物理页号
    paddr_t ppn1 = pte & 0xfff00000;
    paddr_t ppn0 = pte & 0x000ffc00;

    paddr = (ppn1 << 2) | (ppn0 << 2) | voff;
  }
  else {
    paddr_t ppn1 = pte & 0xfff00000;
    paddr = (ppn1 << 2) | (vpn0) | voff;
  }

  // assert(paddr == vaddr);
  return paddr;
}

