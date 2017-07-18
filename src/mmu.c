#include <string.h>
#include <assert.h>
#include "mmu.h"


static const rom_t* rom;
static uint8_t ram[0x800]   = { 0 };   // zeropage,stack,ram
static uint8_t sram[0x2000] = { 0 };


void initmmu(rom_t* const rom_ptr)
{
	rom = rom_ptr;
}


uint_fast8_t mmuread(const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF];
	else if (addr >= ADDR_PRGROM_UPPER)
		return rom->data[(rom->prgrom_num_banks == 1) ? (addr - ADDR_PRGROM_UPPER) : (addr - ADDR_PRGROM)];
	else if (addr >= ADDR_PRGROM)
		return rom->data[addr - ADDR_PRGROM];
	else if (addr >= ADDR_SRAM)
		return sram[addr - ADDR_SRAM];

	return 0;
}


void mmuwrite(const uint_fast8_t value, const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = value;
}

