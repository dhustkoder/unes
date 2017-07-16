#include <string.h>
#include <assert.h>
#include "mem.h"


static const rom_t* rom;
static uint8_t mem[0x800]   = { 0 };   // zeropage,stack,ram
static uint8_t sram[0x2000] = { 0 };


void initmem(rom_t* const rom_ptr)
{
	rom = rom_ptr;
}


uint_fast8_t memread(const uint_fast16_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		return mem[addr&0x7FF];
	else if (addr >= ADDR_PRGROM_UPPER)
		return rom->data[addr - ADDR_PRGROM_UPPER];
	else if (addr >= ADDR_PRGROM)
		return rom->data[addr - ADDR_PRGROM];
	else if (addr >= ADDR_SRAM)
		return sram[addr - ADDR_SRAM];


	return 0;
}


void memwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		mem[addr&0x7FF] = value;
}

