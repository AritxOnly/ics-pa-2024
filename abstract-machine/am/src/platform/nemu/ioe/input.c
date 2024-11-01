#include <am.h>
#include <nemu.h>
#include <stdint.h>

#define KEYDOWN_MASK 0x8000

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  uint8_t cur_key = inw(KBD_ADDR);
  kbd->keydown = cur_key ? 1 : 0;
  kbd->keycode = kbd->keydown ? cur_key : AM_KEY_NONE;
}
