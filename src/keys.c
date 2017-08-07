#include <stdbool.h>
#include "input.h"
#include "keys.h"


static uint8_t keys = 0x00;
static int8_t keyshift = 0;
static bool keystrobe = true;


void joywrite(const uint_fast8_t val)
{
	const bool oldstrobe = keystrobe;
	keystrobe = (val&0x01) != 0;
	if (oldstrobe && !keystrobe) {
		keyshift = 0;
		keys = getkeystate(KEY_RIGHT)<<7  |
		       getkeystate(KEY_LEFT)<<6   |
		       getkeystate(KEY_DOWN)<<5   |
		       getkeystate(KEY_UP)<<4     |
		       getkeystate(KEY_START)<<3  |
		       getkeystate(KEY_SELECT)<<2 |
		       getkeystate(KEY_B)<<1      |
		       getkeystate(KEY_A);
	}
}

uint_fast8_t joyread(const uint_fast16_t addr)
{
	// TODO implement joypad 2
	if (addr == 0x4017)
		return 0x00;

	if (keystrobe)
		return getkeystate(KEY_A);
	else if (keyshift >= 8)
		return 0x01;
	
	const uint_fast8_t k = keys&0x01;
	keys >>= 1;
	++keyshift;
	return k;
}

