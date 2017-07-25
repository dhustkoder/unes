#include <assert.h>
#include "rom.h"
#include "mmu.h"
#include "apu.h"
#include "ppu.h"


static uint8_t ram[0x800] = { 0 };   // zeropage,stack,ram


static void iowrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr >= 0x4000 && addr != 0x4014 && addr <= 0x4017)
		apuwrite(val, addr);
	else if ((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014)
		ppuwrite(val, addr);
}

static uint_fast8_t ioread(const uint_fast16_t addr)
{
	// a read from 0x4017 is joypad's 
	if (addr >= 0x4000 && addr != 0x4014 && addr < 0x4017)
		return apuread(addr);
	else if ((addr >= 0x2000 && addr <= 0x2007) || addr == 0x4014)
		return ppuread(addr);
	return 0x00;
}


uint_fast8_t mmuread(const uint_fast16_t addr)
{
	if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF]; // also handles mirrors
	else if (addr >= ADDR_PRGROM)
		return romread(addr);
	else if (addr < ADDR_EXPROM)
		return ioread(addr);
	return 0x00;
}


void mmuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = val; // also handles mirrors
	else if (addr < ADDR_EXPROM)
		iowrite(val, addr);
}

