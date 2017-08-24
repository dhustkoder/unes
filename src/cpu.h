#ifndef UNES_CPU_H_
#define UNES_CPU_H_
#include <stdint.h>
#include <stdbool.h>


#define NES_CPU_FREQ (1789773)


enum IrqSource {
	IRQ_SRC_APU_FRAME_COUNTER,
	IRQ_SRC_APU_DMC_TIMER,

	IRQ_SRC_SIZE
};

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


extern void resetcpu(void);
extern void request_nmi(void);
extern unsigned stepcpu(void);


static inline void set_irq_source(const enum IrqSource src, const bool value)
{
	extern bool cpu_irq_sources[IRQ_SRC_SIZE];
	cpu_irq_sources[src] = value;
}

static inline bool get_irq_source(const enum IrqSource src)
{
	extern bool cpu_irq_sources[IRQ_SRC_SIZE];
	return cpu_irq_sources[src];
}

static inline void trigger_nmi(void)
{
	extern bool cpu_nmi;
	cpu_nmi = true;
}


#endif
