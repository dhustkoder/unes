#include <string.h>
#include "mem.h"


static const rom_t* rom;
static uint8_t mem[0x800];   // zeropage,stack,ram
static uint8_t sram[0x2000];


void initmem(rom_t* const rom_ptr)
{
	rom = rom_ptr;
	if (rom->prgrom_num_banks > 1) {
		memcpy(&mem[ADDR_PRGROM], rom->data, PRGROM_BANK_SIZE * 2);
	} else {
		memcpy(&mem[ADDR_PRGROM], rom->data, PRGROM_BANK_SIZE);
		memcpy(&mem[ADDR_PRGROM_UPPER], rom->data, PRGROM_BANK_SIZE);
	}
}


uint_fast8_t memread(const uint_fast16_t addr)
{
	if (addr < ADDR_IOREGS1)
		return mem[addr&0x800];
	else if (addr >= ADDR_SRAM && addr < ADDR_PRGROM)
		return sram[addr - ADDR_SRAM];
	return 0;
}

