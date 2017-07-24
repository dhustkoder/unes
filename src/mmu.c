#include <assert.h>
#include "rom.h"
#include "mmu.h"
#include "apu.h"
#include "ppu.h"


static uint8_t ram[0x800] = { 0 };   // zeropage,stack,ram


static void iowrite(const uint_fast8_t val, const int_fast32_t addr)
{
	if (addr >= 0x4000 && addr != 0x4014 && addr <= 0x4017)
		apuwrite(val, addr);
	else if ((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014)
		ppuwrite(val, addr);
}

static uint_fast8_t ioread(const int_fast32_t addr)
{
	if (addr >= 0x4000 && addr != 0x4014 && addr <= 0x4017)
		return apuread(addr);
	else if ((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014)
		return ppuread(addr);
	return 0x00;
}


uint_fast8_t mmuread(const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF]; // also handles mirrors
	else if (addr >= ADDR_PRGROM)
		return romread(addr);
	else if (addr < ADDR_EXPROM)
		return ioread(addr);

	return 0x00;
}


void mmuwrite(const uint_fast8_t val, const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = val; // also handles mirrors
	else if (addr < ADDR_EXPROM)
		iowrite(val, addr);
}

