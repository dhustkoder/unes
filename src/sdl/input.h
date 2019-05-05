#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "SDL.h"
#include "cpu.h"


static inline uint8_t get_pad_state(const joypad_t pad)
{
	extern Uint8 sdl_padstate[2];
	return sdl_padstate[pad];
}



#endif
