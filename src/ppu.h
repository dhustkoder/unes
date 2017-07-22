#ifndef UNES_PPU_H_
#define UNES_PPU_H_
#include <stdint.h>

extern void resetppu(void);
extern void stepppu(void);
extern void ppuwrite(uint_fast8_t val, int_fast32_t addr);
extern uint_fast8_t ppuread(int_fast32_t addr);


#endif
