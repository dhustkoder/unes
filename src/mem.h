#ifndef UNES_MEM_H_
#define UNES_MEM_H_
#include "types.h"


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


extern void initmem(rom_t* rom);
extern uint_fast8_t memread(int_fast32_t addr);
extern void memwrite(uint_fast8_t value, int_fast32_t addr);

static inline uint_fast16_t memread16(const int_fast32_t addr)
{
	return (memread(addr + 1)<<8)|memread(addr);
}

static inline void memwrite16(uint_fast16_t value, const int_fast32_t addr)
{
	memwrite(value&0xFF, addr);
	memwrite((value&0xFF00)>>8, addr + 1);
}

#endif
