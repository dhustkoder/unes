#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "cpu.h"


static enum KeyState getpadstate(const enum Joypad pad)
{
	extern uint8_t sdl2_padstate[JOYPAD_NJOYPADS];
	return sdl2_padstate[pad];
}



#endif
