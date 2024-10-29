#include "tracer.h"
#include "common.h"
#include "debug.h"
#include <stdio.h>
#include <elf.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <utils.h>

extern IRingBuf ring_buf;

Symbol symbols[256] = {};
int symbol_cnt = 0;

void init_iringbuf() {
  memset(ring_buf.inst, 0, sizeof(ring_buf.inst));
  ring_buf.head = 0;
}

void init_ftrace(const char *elf_file) {
  if (elf_file == NULL) {
    return;
  }
    
  FILE *fp = fopen(elf_file, "r");

  // 读取ELF表头
  Elf32_Ehdr ehdr;
  assert(fread(&ehdr, 1, sizeof(ehdr), fp) == sizeof(ehdr));

  // 验证ELF魔数
  Assert(ehdr.e_ident[EI_MAG0] == ELFMAG0 && 
         ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
         ehdr.e_ident[EI_MAG3] == ELFMAG3, 
         "ELF Magic Number evaluation fault, probably because of this is\
          not an ELF file.");

  // 获取节头表
  Elf32_Shdr *shdrs = malloc(ehdr.e_shentsize * ehdr.e_shnum);
  fseek(fp, ehdr.e_shoff, SEEK_SET);
  assert(fread(shdrs, 1, ehdr.e_shentsize * ehdr.e_shnum, fp) == ehdr.e_shentsize * ehdr.e_shnum);

  // .symtab & .strtab节
  Elf32_Shdr *symtab_shdr = NULL;
  Elf32_Shdr *strtab_shdr = NULL;

  // 读取节头字节表
  Elf32_Shdr shstr_shdr = shdrs[ehdr.e_shstrndx];
  char *shstrtab = malloc(shstr_shdr.sh_size);
  fseek(fp, shstr_shdr.sh_offset, SEEK_SET);
  assert(fread(shstrtab, 1, shstr_shdr.sh_size, fp) == shstr_shdr.sh_size);
  
  for (int i = 0; i < ehdr.e_shnum; i++) {
    char *section_name = &shstrtab[shdrs[i].sh_name];
    if (strcmp(section_name, ".symtab") == 0) {
      symtab_shdr = &shdrs[i];
    } else if (strcmp(section_name, ".strtab") == 0) {
      strtab_shdr = &shdrs[i];
    }
  }

  Assert(symtab_shdr != NULL && strtab_shdr != NULL, 
         "'.symtab' or '.strtab' section not found!");
  
  // 读取符号表
  Elf32_Sym *symtab = malloc(symtab_shdr->sh_size);
  fseek(fp, symtab_shdr->sh_offset, SEEK_SET);
  assert(fread(symtab, 1, symtab_shdr->sh_size, fp) == symtab_shdr->sh_size);
  int sym_num = symtab_shdr->sh_size / sizeof(Elf32_Sym);

  // 读取字符串表
  char *strtab = malloc(strtab_shdr->sh_size);
  fseek(fp, strtab_shdr->sh_offset, SEEK_SET);
  assert(fread(strtab, 1, strtab_shdr->sh_size, fp) == strtab_shdr->sh_size);
  
  // 存储符号信息
  for (int i = 0; i < sym_num; i++) {
    Elf32_Sym sym = symtab[i];
    if (ELF32_ST_TYPE(sym.st_info) == STT_FUNC) {
      // 若该符号为函数
      symbols[symbol_cnt].st_value = sym.st_value;
      symbols[symbol_cnt].st_size = sym.st_size;
      symbols[symbol_cnt].st_info = sym.st_info;
      symbols[symbol_cnt].st_name = strdup(&strtab[sym.st_name]);
      ++symbol_cnt;
    }
  }

  free(shdrs);
  free(shstrtab);
  free(symtab);
  free(strtab);
  fclose(fp);
}

#define MAX_CALL_DEPTH 1024

paddr_t call_stack[MAX_CALL_DEPTH];  // 调用栈
int call_depth = 0;

static Symbol *search(paddr_t addr) {
  for (int i = 0; i < symbol_cnt; i++) {
    Symbol *sym = &symbols[i];
    if (addr >= sym->st_value && addr < sym->st_value + sym->st_size) {
      return sym;
    }
  }
  return NULL;
}

void function_call(paddr_t from, paddr_t target) {
  Symbol *sym = search(target);
  char *func_name = sym ? sym->st_name : "???";
  _Log("0x%08x: ", from);
  for (int i = 0; i < call_depth; i++) {
    _Log("\t");
  }
  _Log("call [%s@0x%08x]\n", func_name, target);
  call_stack[call_depth++] = from + 4;
}

void function_return(paddr_t from, paddr_t target) {
  Symbol *sym = search(target);
  char *func_name = sym ? sym->st_name : "???";
  _Log("0x%08x: ", from);
  for (int i = 0; i < call_depth; i++) {
    _Log("\t");
  }
  _Log("ret [%s]\n", func_name);
  call_depth--;
}
