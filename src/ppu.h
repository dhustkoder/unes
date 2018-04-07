#ifndef UNES_PPU_H_
#define UNES_PPU_H_
#include <stdint.h>

extern void resetppu(void);
extern void stepppu(unsigned pputicks);
extern void log_ppu_state(void);
extern void ppuwrite(uint8_t val, uint16_t addr);
extern uint8_t ppuread(uint16_t addr);

#endif
