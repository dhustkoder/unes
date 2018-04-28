#ifndef UNES_CPU_H_
#define UNES_CPU_H_
#include <stdint.h>
#include <stdbool.h>


#define NES_CPU_FREQ (1789773)

typedef uint8_t irq_source_t;
enum IrqSource {
	IRQ_SRC_APU_FRAME_COUNTER,
	IRQ_SRC_APU_DMC_TIMER,

	IRQ_SRC_SIZE
};

typedef uint8_t joypad_t;
enum Joypad {
	JOYPAD_ONE,
	JOYPAD_TWO
};

typedef uint8_t key_state_t;
enum KeyState {
	KEYSTATE_UP,
	KEYSTATE_DOWN
};

typedef uint8_t joykey_t;
enum JoyKey {
	KEY_A,
	KEY_B,
	KEY_SELECT,
	KEY_START,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT
};


extern void cpu_reset(void);
extern unsigned cpu_step(void);
extern void cpu_log_state(void);


#endif
