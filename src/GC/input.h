#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <gccore.h>
#include <ogcsys.h>
#include "cpu.h"


static uint8_t getpadstate(const enum Joypad pad)
{
	((void)pad);
	PAD_ScanPads();
	const int buttons = PAD_ButtonsHeld(0);
	return 
	 ((buttons&PAD_BUTTON_A) > 0)<<KEY_A          |
	 ((buttons&PAD_BUTTON_B) > 0)<<KEY_B          |
	 ((buttons&PAD_TRIGGER_Z) > 0)<<KEY_SELECT    |
	 ((buttons&PAD_BUTTON_START) > 0)<<KEY_START  |
	 ((buttons&PAD_BUTTON_LEFT) > 0)<<KEY_LEFT    |
	 ((buttons&PAD_BUTTON_RIGHT) > 0)<<KEY_RIGHT  |
	 ((buttons&PAD_BUTTON_UP) > 0)<<KEY_UP        |
	 ((buttons&PAD_BUTTON_DOWN) > 0)<<KEY_DOWN;
}



#endif
