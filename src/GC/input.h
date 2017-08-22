#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include "cpu.h"


static uint8_t getpadstate(const enum Joypad pad)
{
	extern uint8_t gc_padstate[JOYPAD_NJOYPADS];
	return gc_padstate[pad];
}



#endif
