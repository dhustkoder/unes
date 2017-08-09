#include <string.h>
#include <stdbool.h>
#include "input.h"
#include "joypad.h"


static uint8_t keys[2] = { 0x00 };
static int8_t keyshift[2] = { 0x00 };
static bool keystrobe = true;


void joywrite(const uint_fast8_t val)
{
	const bool oldstrobe = keystrobe;
	keystrobe = (val&0x01) != 0;
	if (oldstrobe && !keystrobe) {
		memset(keyshift, 0, sizeof(keyshift));
		for (int i = JOYPAD_ONE; i < JOYPAD_NJOYPADS; ++i) {
			keys[i] = getkeystate(i, KEY_RIGHT)<<7  |
				  getkeystate(i, KEY_LEFT)<<6   |
				  getkeystate(i, KEY_DOWN)<<5   |
				  getkeystate(i, KEY_UP)<<4     |
				  getkeystate(i, KEY_START)<<3  |
				  getkeystate(i, KEY_SELECT)<<2 |
				  getkeystate(i, KEY_B)<<1      |
				  getkeystate(i, KEY_A);
		}
	}
}

uint_fast8_t joyread(const uint_fast16_t addr)
{
	assert(addr == 0x4016 || addr == 0x4017);
	const enum Joypad pad = addr == 0x4016 ? JOYPAD_ONE : JOYPAD_TWO;

	if (keystrobe)
		return getkeystate(pad, KEY_A);
	else if (keyshift[pad] >= 8)
		return 0x01;
	
	const uint_fast8_t k = keys[pad]&0x01;
	keys[pad] >>= 1;
	++keyshift[pad];
	return k;
}

