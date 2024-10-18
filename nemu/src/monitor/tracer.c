#include "tracer.h"
#include "debug.h"
#include <string.h>

extern IRingBuf ring_buf;

Symbol symbols = {};

void init_iringbuf() {
    memset(ring_buf.inst, 0, sizeof(ring_buf.inst));
    ring_buf.head = 0;
}

void init_ftrace(const char *elf_file) {
    if (elf_file == NULL) {
        return;
    }
}