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

#include "common.h"
#include "local-include/reg.h"
#include "macro.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <stdint.h>

#define R(i) gpr(i)
#define CSR(i) csr(i)
#define Mr vaddr_read
#define Mw vaddr_write

void function_call(paddr_t from, paddr_t target);
void function_return(paddr_t from, paddr_t target);

enum {
  TYPE_I, TYPE_U, TYPE_S, 
  TYPE_J, // for jal inst
  TYPE_R, TYPE_B,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immJ() do { *imm = SEXT((BITS(i, 31, 31) << 20) | \
                                (BITS(i, 19, 12) << 12) | \
                                (BITS(i, 20, 20) << 11) | \
                                (BITS(i, 30, 21) << 1), 21); } while (0)
#define immB() do { *imm = SEXT((BITS(i, 31, 31) << 12) | \
                                (BITS(i, 7, 7) << 11) | \
                                (BITS(i, 30, 25) << 5) | \
                                (BITS(i, 11, 8) << 1), 13); } while (0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  // Log("rs1=%d, rs2=%d, *rd=%d", rs1, rs2, *rd);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_J:                   immJ(); break;
  }
}

static void jal(int rd, word_t imm, Decode *s);
static void jalr(int rd, word_t imm, word_t src1, Decode *s);
static void recover_mstatus();

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();

  // INSTPAT("0000000 00000 00001 000 00000 11001 11", ret    , N, NEMUTRAP(s->pc, 0));

  // 算术运算（寄存器）：R-type
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);   // ADD
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);   // SUB
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , R, R(rd) = src1 ^ src2);   // XOR
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , R, R(rd) = src1 | src2);   // OR
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , R, R(rd) = src1 & src2);   // AND
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , R, R(rd) = src1 << src2);  // Shift Left Logical
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , R, R(rd) = src1 >> src2);  // Shift Right Logical
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , R, R(rd) = (int32_t)src1 >> src2); // Shift Right Arith
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = ((int32_t)src1 < (int32_t)src2) ? 1 : 0); // Set Less Than
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (src1 < src2) ? 1 : 0); // Set Less Than(U)

  // 算术运算（立即数）：I-type
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);    // ADD immediate
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) = src1 ^ imm);    // XOR immediate
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);    // OR immediate
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) = src1 & imm);    // AND immediate
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli   , I, R(rd) = src1 << BITS(imm, 4, 0));   // Left Shift Logical immediate
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, R(rd) = src1 >> BITS(imm, 4, 0));   // Right Shift Logical immediate
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, R(rd) = (int32_t)src1 >> BITS(imm, 4, 0)); // Right Shift Arith immediate
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = ((int32_t)src1 < (int32_t)imm) ? 1 : 0);  // Set Less Than immediate
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (src1 < imm) ? 1 : 0);  // Set Less Than(U) Than immediate

  // 算术运算（RV32M拓展）：R-type
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = (word_t)((int64_t)(int32_t)src1 * (int64_t)(int32_t)src2) & 0xFFFFFFFF);  // MUL
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , R, R(rd) = (word_t)(((int64_t)(int32_t)src1 * (int64_t)(int32_t)src2) >> 32));  // MUL High
  INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulsu  , R, R(rd) = (word_t)(((int64_t)(int32_t)src1 * (uint64_t)src2) >> 32)); // MUL High(S)(U)
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulu   , R, R(rd) = (word_t)(((uint64_t)src1 * (uint64_t)src2) >> 32)); // MUL High(U)
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div    , R, R(rd) = (int32_t)src1 / (int32_t)src2);
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu   , R, R(rd) = src1 / src2);
  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , R, R(rd) = (int32_t)src1 % (int32_t)src2); // Remainder
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , R, R(rd) = src1 % src2);

  // 内存读取：I-type
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT(Mr(src1 + imm, 1), 8));  // Load byte
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2), 16));  // Load half
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = SEXT(Mr(src1 + imm, 4), 32)); // Load word
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2)); // Load half(U)

  // 内存写入：S-type
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2)); // Store half
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));

  // 分支语句：B-Type
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, if (src1 == src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, if ((int32_t)src1 < (int32_t)src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, if ((int32_t)src1 >= (int32_t)src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, if (src1 < src2) s->dnpc = s->pc + imm);
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, if (src1 >= src2) s->dnpc = s->pc + imm); 

  // 跳转
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, 
          s->dnpc = s->pc + imm; 
          R(rd) = s->snpc; 
          IFDEF(CONFIG_FTRACE, function_call(s->pc, s->dnpc));); // Jump and Link
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, 
            s->dnpc = (imm + src1) & ~1U; 
            R(rd) = s->snpc; 
            IFDEF(CONFIG_FTRACE, 
                  int rs1 = BITS(s->isa.inst.val, 19, 15);
                  if (rd == 0 && rs1 == 1 && imm == 0) {
                    function_return(s->pc, s->dnpc);
                  } else {
                    function_call(s->pc, s->dnpc);
                  });); // Jump and Link Reg

  // 高位立即数操作：U-type
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);

  // CSR指令：I-type
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw, I, if (rd != 0) { R(rd) = CSR(imm); } CSR(imm) = src1);
  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs, I, if (rd != 0) { R(rd) = CSR(imm); } CSR(imm) = CSR(imm) | src1);

  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , N, s->dnpc = isa_raise_intr(R(MUXDEF(__riscv_e, 15/* a5 */, 17/* a7 */)), s->pc)); // Transfer control to OS
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0  // Transfer control to debugger
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , N, s->dnpc = CSR(MEPC); recover_mstatus());
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}

static inline void jal(int rd, word_t imm, Decode *s) {
  s->dnpc = s->pc + imm; 
  R(rd) = s->snpc; 
  IFDEF(CONFIG_FTRACE, function_call(s->pc, s->dnpc));
}

static inline void jalr(int rd, word_t imm, word_t src1, Decode *s) {
  s->dnpc = (imm + src1) & ~1U; 
  R(rd) = s->snpc; 
  IFDEF(CONFIG_FTRACE, 
        int rs1 = BITS(s->isa.inst.val, 19, 15);
        if (rd == 0 && rs1 == 1 && imm == 0) {
          function_return(s->pc, s->dnpc);
        } else {
          function_call(s->pc, s->dnpc);
        });
}

static inline void recover_mstatus() {
  CSR(MSTATUS) = CSR(MSTATUS) & 0xfffffff7;
  CSR(MSTATUS) = CSR(MSTATUS) | ((CSR(MSTATUS) >> 7) & 1) << 3;
  CSR(MSTATUS) = CSR(MSTATUS) | 0x80;
}
