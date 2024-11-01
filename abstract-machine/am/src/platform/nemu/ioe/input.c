#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint16_t cur_key = inw(KBD_ADDR);
  kbd->keydown = cur_key & 0xff;
  kbd->keycode = cur_key >> 15;
}
