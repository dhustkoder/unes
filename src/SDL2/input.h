#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "keys.h"


static inline enum KeyState getkeystate(const enum Key key)
{
	assert(key >= 0 && key < KEY_NKEYS);
	extern enum KeyState keys_state[KEY_NKEYS];
	return keys_state[key];
}



#endif
