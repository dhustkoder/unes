#include <stdio.h>
#include <assert.h>
#include "rom.h"
#include "mmu.h"
#include "apu.h"
#include "ppu.h"


static uint8_t ram[0x800]; // zeropage,stack,ram
static uint8_t gdb[0x2000] = { 0 }; // used for test roms text


static void iowrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr >= 0x4000 && addr <= 0x4017) {
		if (addr != 0x4014)
			apuwrite(val, addr);
		else
			ppuwrite(val, addr);
	} else if (addr >= 0x2000 && addr < 0x4000) {
		ppuwrite(val, addr&0x07);
	} else {
		assert(printf("$%.4lX\n", addr) && false && "UNKNOWN ADDRESS");
	}
}

static uint_fast8_t ioread(const uint_fast16_t addr)
{
	if (addr >= 0x4000 && addr <= 0x4017) {
		if (addr < 0x4016) {
			if (addr != 0x4014)
				return apuread(addr);
			else
				return ppuread(addr);
		}
		// $4016 - $4017 is joypad's
	} else if (addr >= 0x2000 && addr <= 0x4000) {
		return ppuread(addr&0x07);
	} else {
		assert(printf("$%.4lX\n", addr) && false && "UNKNOWN ADDRESS");
	}
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
	else if (addr < ADDR_PRGROM)
		gdb[addr - ADDR_SRAM] = val;
}

