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

#define PGMASK ~(PGSIZE - 1)

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
      uint32_t vaddr      = phdr->p_vaddr;
      uint32_t mem_size   = phdr->p_memsz;
      uint32_t file_size  = phdr->p_filesz;
      uint32_t offset     = phdr->p_offset;

      // fseek(fp, offset, SEEK_SET);
      fs_lseek(fd, offset, SEEK_SET);

      // 对齐
      uint32_t page_start = vaddr & PGMASK;
      uint32_t page_end   = (vaddr + mem_size + PGSIZE - 1) & PGMASK;

      uint32_t inpg_offset = vaddr & ~PGMASK;

      uint32_t loaded = 0;
      uint32_t seg_size = page_end - page_start;

      while (loaded < seg_size) {
        uint32_t page_vaddr = page_start + loaded;

        // 分配一页物理内存
        void *pa = new_page(1); // 每次分配一页(4096字节)
        assert(pa);

        // 将这页映射到用户进程地址空间中
        map(&(pcb->as), (void *)page_vaddr, pa, 0b111);

        Log("assigned %p to %p", page_vaddr, pa);

        uint32_t page_bytes = PGSIZE;  // 4KB
        // 但最后一页可能只需要一部分
        if (loaded + page_bytes > seg_size) {
          page_bytes = seg_size - loaded;
        }

        memset(pa, 0, PGSIZE);

        uint32_t page_file_start = loaded - (vaddr - page_start); 

        uint32_t file_part = 0;
        if (page_file_start < file_size) {
          // 剩余可读 = file_size - page_file_start
          file_part = file_size - page_file_start;
          if (file_part > page_bytes) {
            file_part = page_bytes;
          }
        }
        
        uint32_t page_inner_offset = 0; 
        if (loaded == 0) {
          page_inner_offset = inpg_offset;
        }

        if (file_part > 0) {
          size_t nr = fs_read(fd, (char *)pa + page_inner_offset, file_part);
          assert(nr == file_part);
        }

        loaded += page_bytes;
      }

      // int ret = fread((void *)(uintptr_t)vaddr, 1, file_size, fp);
      // size_t ret = ramdisk_read((void *)(uintptr_t)vaddr, offset, file_size);
      // size_t ret = fs_read(fd, (void *)(uintptr_t)vaddr, file_size);
      // assert(ret == file_size);

      // if (mem_size > file_size) {
      //   memset((void *)(uintptr_t)(vaddr + file_size), 0, mem_size - file_size);
      //   // 清零
      // }

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

#define UNSPECIFIED_MEMORY 0
#define MEMORY_SPACE sizeof(void *)

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  protect(&pcb->as);

  Area kstack = {
    .start = pcb->stack,
    .end   = pcb->stack + STACK_SIZE,
  };

  uintptr_t entry = loader(pcb, filename);

  if (!entry) {
    Log("Loaded entry is NULL, doing nothing...");
    return;
  }

  int num_pages = STACK_SIZE / PGSIZE;  

  void *user_stack_va_base = (char *)pcb->as.area.end - STACK_SIZE;
  void *user_stack_base    = new_page(num_pages);

  for (int i = 0; i < num_pages; i++) {
    void *page_pa = (char *)user_stack_base    + i * PGSIZE;
    void *page_va = (char *)user_stack_va_base + i * PGSIZE;

    map(&pcb->as, page_va, page_pa, 0b111);
  }

  uintptr_t sp   = (uintptr_t)user_stack_base    + STACK_SIZE - 1;  // 物理地址栈顶
  uintptr_t vasp = (uintptr_t)user_stack_va_base + STACK_SIZE - 1;  // 虚拟地址栈顶

  sp   -= UNSPECIFIED_MEMORY;
  vasp -= UNSPECIFIED_MEMORY;

  Log("Current user stack: sp = %p, vasp = %p", sp, vasp);

  int argc, envc;

  if (!argv) { argc = 0; }
  if (!envp) { envc = 0; }

  for (argc = 0; argv && argv[argc] != NULL; argc++) ;
  for (envc = 0; envp && envp[envc] != NULL; envc++) ;

  char **tmp_argv = malloc(argc * sizeof(char *));
  char **tmp_envp = malloc(envc * sizeof(char *));

  for (int i = 0; i < argc; i++) {
    int len = strlen(argv[i]) + 1;
    strncpy((char *)sp, argv[i], len);
    tmp_argv[i] = (char *)sp;

    sp   -= len;
    vasp -= len;
  }

  for (int i = 0; i < envc; i++) {
    int len = strlen(envp[i]) + 1;
    strncpy((char *)sp, envp[i], len);
    tmp_envp[i] = (char *)sp;

    sp   -= len;
    vasp -= len;
  }

  typedef char ** space;

  sp -= UNSPECIFIED_MEMORY;
  sp -= MEMORY_SPACE;

  vasp -= UNSPECIFIED_MEMORY;
  vasp -= MEMORY_SPACE;

  *(space)sp = NULL;

  for (int i = envc - 1; i >= 0; i--) {
    sp   -= MEMORY_SPACE;
    vasp -= MEMORY_SPACE;
    *(space)sp = tmp_envp[i];
  }
  sp   -= MEMORY_SPACE;
  vasp -= MEMORY_SPACE;
  *(space)sp = NULL;

  for (int i = argc - 1; i >= 0; i--) {
    sp   -= MEMORY_SPACE;
    vasp -= MEMORY_SPACE;
    *(space)sp = tmp_argv[i];
  }
  vasp -= MEMORY_SPACE;
  sp   -= MEMORY_SPACE;
  *(int *)sp = argc;

  Context *context = ucontext(&pcb->as, kstack, (void *)entry);

  pcb->cp = context;
  pcb->cp->GPRx = sp;

  Log("mepc: %p", pcb->cp->mepc);
}