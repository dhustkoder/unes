#include <stdio.h>
#include <assert.h>
#include "rom.h"
#include "mmu.h"
#include "apu.h"
#include "ppu.h"


static uint8_t ram[0x800]; // zeropage,stack,ram


static void iowrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr >= 0x4000 && addr <= 0x4017) {
		if (addr == 0x4014)
			ppuwrite(val, addr);
		else
			apuwrite(val, addr);
	} else if (addr >= 0x2000 && addr < 0x4000) {
		ppuwrite(val, addr);
	} else {
		assert(printf("IO WRITE: $%.4lX\n", addr) && false);
	}
}

static uint_fast8_t ioread(const uint_fast16_t addr)
{
	if (addr >= 0x4000 && addr <= 0x4016) {
		if (addr == 0x4014)
			return ppuread(addr);
		else
			return apuread(addr);
	} else if (addr >= 0x4016 && addr <= 0x4017) {
		// read from $4016 - $4017 is joypad's
	} else if (addr >= 0x2000 && addr <= 0x4000) {
		return ppuread(addr);
	} else {
		assert(printf("IO READ: $%.4lX\n", addr) && false);
	}
	return 0x00;
}


uint_fast8_t mmuread(const uint_fast16_t addr)
{
	if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF];
	else if (addr >= ADDR_SRAM)
		return romread(addr);
	else if (addr < ADDR_EXPROM)
		return ioread(addr);
	return 0x00;
}


void mmuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = val;
	else if (addr < ADDR_EXPROM)
		iowrite(val, addr);
	else if (addr >= ADDR_SRAM)
		romwrite(val, addr);
}

