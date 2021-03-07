#ifndef UNES_H_
#define UNES_H_
#include "platform.h"

#define NES_CPU_FREQ     (1789773)
#define NES_SCR_WIDTH    (256)
#define NES_SCR_HEIGHT   (240)

#define PRGROM_BANK_SIZE (0x4000)
#define CHR_BANK_SIZE    (0x2000)
#define SRAM_BANK_SIZE   (0x2000)
#define TRAINER_SIZE     (0x0200)


typedef uint8_t irq_source_t;
enum IrqSource {
	IRQ_SRC_APU_FRAME_COUNTER,
	IRQ_SRC_APU_DMC_TIMER,

	IRQ_SRC_COUNT
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
	KEY_RIGHT,
	
	KEY_COUNT
};


typedef uint8_t mapper_type_t;
enum MapperType {
	MAPPER_TYPE_NROM,
	MAPPER_TYPE_MMC1,

	MAPPER_TYPE_COUNT
};

typedef uint8_t nt_mirroring_mode_t;
enum NTMirroringMode {
	NT_MIRRORING_MODE_HORIZONTAL,
	NT_MIRRORING_MODE_VERTICAL,
	NT_MIRRORING_MODE_ONE_SCREEN_LOW,
	NT_MIRRORING_MODE_ONE_SCREEN_UPPER,

	NT_MIRRORING_MODE_COUNT
};





static inline uint8_t get_pad_state(const joypad_t pad)
{
	extern uint8_t unes_pad_states[2];
	return unes_pad_states[pad];
}


extern bool rom_load(const uint8_t* data);
extern void rom_unload(void);
extern void rom_write(uint8_t value, uint16_t addr);


extern void cpu_reset(void);
extern short cpu_step(void);


extern void ppu_reset(void);
extern void ppu_step(short pputicks);
extern void ppu_write(uint8_t val, uint16_t addr);
extern uint8_t ppu_read(uint16_t addr);


extern void apu_reset(void);
extern void apu_step(short aputicks);
extern void apu_write(uint8_t val, uint16_t addr);
extern uint8_t apu_read_status(void);



#endif
