#ifndef UNES_PPU_H_
#define UNES_PPU_H_
#include <stdint.h>

extern void resetppu(void);
extern void stepppu(unsigned pputicks);
extern void ppuwrite(uint_fast8_t val, uint_fast16_t addr);
extern uint_fast8_t ppuread(uint_fast16_t addr);

#endif
