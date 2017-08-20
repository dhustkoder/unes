#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "SDL.h"
#include "cpu.h"


static enum KeyState getpadstate(const enum Joypad pad)
{
	extern Uint8 sdl2_padstate[JOYPAD_NJOYPADS];
	return sdl2_padstate[pad];
}



#endif
