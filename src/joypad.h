#ifndef UNES_JOYPAD_H_
#define UNES_JOYPAD_H_
#include <stdint.h>


enum Joypad {
	JOYPAD_ONE,
	JOYPAD_TWO,
	JOYPAD_NJOYPADS
};

enum KeyState {
	KEYSTATE_UP,
	KEYSTATE_DOWN,
	KEYSTATE_NKEYSTATES
};

enum Key {
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_NKEYS
};


extern void joywrite(uint_fast8_t val);
extern uint_fast8_t joyread(uint_fast16_t addr);


#endif
