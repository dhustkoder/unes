#ifndef UNES_MMU_H_
#define UNES_MMU_H_
#include "rom.h"


#define ADDR_RESET_VECTOR (0xFFFC)
#define ADDR_PRGROM_UPPER (0xC000)
#define ADDR_PRGROM       (0x8000)
#define ADDR_SRAM         (0x6000)
#define ADDR_EXPROM       (0x4020)
#define ADDR_IOREGS2      (0x4000)
#define ADDR_MIRRORS2     (0x2008)
#define ADDR_IOREGS1      (0x2000)
#define ADDR_MIRRORS1     (0x0800)
#define ADDR_RAM          (0x0200)
#define ADDR_STACK        (0x0100)
#define ADDR_ZEROPAGE     (0x0000)


extern void initmmu(rom_t* rom);
extern uint_fast8_t mmuread(int_fast32_t addr);
extern void mmuwrite(uint_fast8_t value, int_fast32_t addr);

static inline uint_fast16_t mmuread16(const int_fast32_t addr)
{
	return (mmuread(addr + 1)<<8)|mmuread(addr);
}

static inline void mmuwrite16(const uint_fast16_t value, const int_fast32_t addr)
{
	mmuwrite(value&0xFF, addr);
	mmuwrite((value&0xFF00)>>8, addr + 1);
}

#endif
