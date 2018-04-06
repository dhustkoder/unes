#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "SDL.h"
#include "cpu.h"


static inline uint8_t getpadstate(const joypad_t pad)
{
	extern Uint8 sdl2_padstate[2];
	return sdl2_padstate[pad];
}



#endif
