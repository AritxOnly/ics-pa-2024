#include "tracer.h"
#include <string.h>

extern IRingBuf ring_buf; 

void init_iringbuf() {
    memset(ring_buf.inst, 0, sizeof(ring_buf.inst));
    ring_buf.head = 0;
}