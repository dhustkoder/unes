#ifndef UNES_KEYS_H_
#define UNES_KEYS_H_
#include <stdint.h>


typedef uint_fast8_t KeyState;
enum {
	KEYSTATE_UP = 0,
	KEYSTATE_DOWN = 1
};

typedef uint_fast8_t Key;
enum {
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
