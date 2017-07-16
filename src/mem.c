#include <string.h>
#include "mem.h"


static uint8_t mem[0x10000] = { 0 };


void initmem(const rom_t* const rom)
{
	if (rom->prgrom_num_banks > 1) {
		memcpy(&mem[ADDR_PRGROM], rom->data, PRGROM_BANK_SIZE * 2);
	} else {
		memcpy(&mem[ADDR_PRGROM], rom->data, PRGROM_BANK_SIZE);
		memcpy(&mem[ADDR_PRGROM_UPPER], rom->data, PRGROM_BANK_SIZE);
	}
}


uint_fast8_t memread(const uint_fast16_t addr)
{
	return mem[addr];
}

