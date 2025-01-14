#include <proc.h>
#include <elf.h>
#include <fs.h>

size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t get_ramdisk_size();

/* ISA宏 */
#if defined(__ISA_AM_NATIVE__)
  # define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
  # define EXPECT_TYPE EM_386
#elif defined(__ISA_MIPS32__)
  #define EXPECT_TYPE EM_MIPS
#elif defined(__riscv)
  #define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_LOONGARCH32R__)
  #define EXPECT_TYPE 258 // 见龙芯ABI参考：
  /* https://loongson.github.io/LoongArch-Documentation/LoongArch-ELF-ABI-CN.html */
#else
# error unsupported ISA __ISA__
#endif

/* 寻址空间宏 */
#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

static uintptr_t loader(PCB *pcb, const char *filename) {
  int fd = fs_open(filename, 0, 0);

  Elf_Ehdr ehdr;  // ELF Header
  assert(fs_read(fd, &ehdr, sizeof(ehdr)) == sizeof(ehdr));
  // assert(ramdisk_read(&ehdr, 0, sizeof(ehdr)) == sizeof(ehdr));

  if (!(ehdr.e_ident[EI_MAG0] == ELFMAG0 && 
        ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
        ehdr.e_ident[EI_MAG2] == ELFMAG2 &&
        ehdr.e_ident[EI_MAG3] == ELFMAG3)) {
    panic("ELF Magic Number evaluation fault, probably because of this is\
          not an ELF file.");
  }

  /* 判断ELF文件ISA信息 */
  if (ehdr.e_machine != EXPECT_TYPE) {
    panic("ELF image machine type doesn't correspond to NEMU ISA.");
  }

  Elf_Phdr *phdrs = malloc(ehdr.e_phnum * ehdr.e_phentsize);
  fs_lseek(fd, ehdr.e_phoff, SEEK_SET);
  // if (ramdisk_read(phdrs, ehdr.e_phoff, 
  //              ehdr.e_phnum * ehdr.e_phentsize) 
  //               != ehdr.e_phnum * ehdr.e_phentsize) {
  if (fs_read(fd, phdrs, 
          ehdr.e_phnum * ehdr.e_phentsize)
          != ehdr.e_phnum * ehdr.e_phentsize) {
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

      // fseek(fp, offset, SEEK_SET);
      fs_lseek(fd, offset, SEEK_SET);
      // int ret = fread((void *)(uintptr_t)vaddr, 1, file_size, fp);
      // size_t ret = ramdisk_read((void *)(uintptr_t)vaddr, offset, file_size);
      size_t ret = fs_read(fd, (void *)(uintptr_t)vaddr, file_size);
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

  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", (void *)entry);
  ((void(*)())entry) ();
}

void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  // Log("pcb = %p", pcb);

  // 通过 pcb->stack 来提供栈区域
  Area kstack = {
    .start = pcb->stack,
    .end   = pcb->stack + STACK_SIZE,
  };

  // 调用 kcontext() 在这片栈区里创建上下文
  Context *context = kcontext(kstack, entry, arg);

  // 记录到 pcb->cp 里
  pcb->cp = context;
}
