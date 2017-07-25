#ifndef UNES_PPU_H_
#define UNES_PPU_H_
#include <stdint.h>

extern void resetppu(void);
extern void stepppu(void);
extern void ppuwrite(uint_fast8_t val, uint_fast16_t addr);
extern uint_fast8_t ppuread(uint_fast16_t addr);
extern void ppu_load_chr_rom(const uint8_t* chr_rom);

#endif
