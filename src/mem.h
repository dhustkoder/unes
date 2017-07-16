#ifndef UNES_MEM_H_
#define UNES_MEM_H_
#include "types.h"


#define ADDR_RESET_VECTOR (0xFFFC)
#define ADDR_PRGROM_UPPER (0xC000)
#define ADDR_PRGROM       (0x8000)
#define ADDR_SRAM         (0x6000)
#define ADDR_EXPROM       (0x4020)
#define ADDR_IOREGS2      (0x4000)
#define ADDR_MIRROR2      (0x2008)
#define ADDR_IOREGS1      (0x2000)
#define ADDR_MIRROR1      (0x0800)
#define ADDR_RAM          (0x0200)
#define ADDR_STACK        (0x0100)
#define ADDR_ZEROPAGE     (0x0000)


extern void initmem(const rom_t* rom);
extern uint_fast8_t memread(uint_fast16_t addr);


static inline uint_fast16_t memread16(const uint_fast16_t addr)
{
	return (memread(addr + 1)<<8)|memread(addr);
}


#endif
