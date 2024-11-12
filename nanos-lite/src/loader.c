#include <proc.h>
#include <elf.h>

#include <stdio.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  // TODO();
  FILE *fp = NULL;
  if (filename != NULL) {
    fp = fopen(filename, "r");
  } else {
    fp = fopen("ramdisk.img", "r");
  }
  assert(fp != NULL);

  Elf_Ehdr ehdr;  // ELF Header
  assert(fread(&ehdr, 1, sizeof(ehdr), fp) == sizeof(ehdr));

  if (!(ehdr.e_ident[EI_MAG0] == ELFMAG0 && 
        ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr.e_ident[EI_MAG3] == ELFMAG3)) {
    panic("ELF Magic Number evaluation fault, probably because of this is\
          not an ELF file.");
  }

  Elf_Phdr *phdrs = malloc(ehdr.e_phnum * ehdr.e_phentsize);
  fseek(fp, ehdr.e_phoff, SEEK_SET);
  if (fread(phdrs, ehdr.e_phentsize, ehdr.e_phnum, fp) != ehdr.e_phnum) {
    panic("Failed to read program headers!!!");
  }

  for (int i = 0; i < ehdr.e_phnum; i++) {
    Elf_Phdr *phdr = &phdrs[i];
    if (phdr->p_type == PT_LOAD) {
      uint32_t vaddr = phdr->p_vaddr;
      // uint32_t paddr = phdr->p_paddr;
      uint32_t mem_size = phdr->p_memsz;
      uint32_t file_size = phdr->p_filesz;
      uint32_t offset = phdr->p_offset;

      fseek(fp, offset, SEEK_SET);
      int ret = fread((void *)(uintptr_t)vaddr, 1, file_size, fp);
      assert(ret == file_size);

      if (mem_size > file_size) {
        memset((void *)(uintptr_t)(vaddr + file_size), 0, mem_size - file_size);
        // 清零
      }

      Log("Loaded segment %d: vaddr = 0x%08x, memsz = 0x%08x, filesz = 0x%08x", 
          i, vaddr, mem_size, file_size);
    }
  }

  free(phdrs);
  fclose(fp);

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", (void *)entry);
  ((void(*)())entry) ();
}

