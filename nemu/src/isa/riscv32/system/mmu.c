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
  int satp_val = csr(SATP);
  int mode     = satp_val >> 31;
  
  if (mode == 0) {
    // Bare mode
    return MMU_DIRECT;
  } else if (mode == 1) {
    // Sv32 mode
    return MMU_TRANSLATE;
  }

  return MMU_FAIL;
}

paddr_t isa_mmu_translate(vaddr_t vaddr, int len, int type) {
  // 拆分虚拟地址
  paddr_t vpn1 = (vaddr >> 22) & 0x3ff;  // 高10位
  paddr_t vpn0 = (vaddr >> 12) & 0x3ff;  // 中10位
  paddr_t voff =  vaddr        & 0xfff;  // 低12位 (页内偏移)

  // 从 satp 中获取根页表物理页号(PPN)
  paddr_t satp_val = csr(SATP);          // 读寄存器
  paddr_t root_ppn = satp_val >> 10;

  // 计算一级页表项基址
  paddr_t root_pt_base = ((paddr_t)root_ppn << 12);

  paddr_t pde_addr = root_pt_base + vpn1 * 4;

  // 读取一级页表项 (PTE1)
  word_t pde = host_read(guest_to_host(pde_addr), 4);

  // 检查一级页表项有效位(V=1)；若无效则抛出异常
  if ((pde & 1) == 0) {
    panic("invalid PDE: vaddr=0x%x, pde_addr=0x%x, pde=0x%x\n",
          vaddr, pde_addr, pde);
  }

  bool leaf1 = (((pde >> 1) & 0x7) != 0);
  if (leaf1) {
    // 如果为叶子
    uint32_t pde_ppn     = pde >> 10;
    paddr_t  pde_base    = (paddr_t)pde_ppn << 12;

    paddr_t  page_offest = (vaddr & 0x003fffff);

    return pde_base + page_offest;
  } else {
    // 读二级页表
    uint32_t pde_ppn           = pde >> 10;
    paddr_t  second_level_base = (paddr_t)pde_ppn << 12;

    paddr_t  pte_addr          = second_level_base + (vpn0 * 4);
    word_t   pte               = host_read(guest_to_host(pte_addr), 4);

    if ((pte & 0x1) == 0) {
      panic("invalid PTE: vaddr=0x%x, pte_addr=0x%x, pte=0x%x\n",
          vaddr, pte_addr, pte);
    }

    bool leaf2 = (((pte >> 1) & 0x7) != 0);
    if (!leaf2) {
      panic("No 3-level page table in Sv32. PDE/PTE not leaf => invalid.\n");
    }

    uint32_t pte_ppn  = pte >> 10;
    paddr_t  pte_base = (paddr_t)pte_ppn << 12;

    return pte_base + voff;
  }
}

