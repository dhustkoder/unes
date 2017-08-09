#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "joypad.h"


static inline enum KeyState getkeystate(const enum Joypad pad, const enum Key key)
{
	extern uint8_t keys_state[JOYPAD_NJOYPADS][KEY_NKEYS];
	return keys_state[pad][key];
}



#endif
