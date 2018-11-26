#ifndef UNES_PPU_H_
#define UNES_PPU_H_
#include <stdint.h>


#define NES_SCR_WIDTH  (256)
#define NES_SCR_HEIGHT (240)

extern void ppu_reset(void);
extern void ppu_step(unsigned pputicks);
extern void ppu_log_state(void);
extern void ppu_write(uint8_t val, uint16_t addr);
extern uint8_t ppu_read(uint16_t addr);

#endif
