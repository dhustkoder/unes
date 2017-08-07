#ifndef UNES_INPUT_H_
#define UNES_INPUT_H_
#include <assert.h>
#include "keys.h"


static inline KeyState getkeystate(const Key key)
{
	extern KeyState keys_state[KEY_NKEYS];
	return keys_state[key];
}



#endif
