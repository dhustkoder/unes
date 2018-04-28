#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include "cpu.h"


static uint8_t get_pad_state(const joypad_t pad)
{
	extern uint8_t gc_nes_padstate[2];
	return gc_nes_padstate[pad];
}



#endif
