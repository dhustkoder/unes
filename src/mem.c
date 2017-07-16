#include "mem.h"


static uint8_t mem[0x10000] = { 0 };


void initmem(const rom_t* const rom)
{
	memcpy(&mem[ADDR_PRGROM], rom->data, rom->prgrom_size);
}


uint_fast8_t read8(const uint_fast16_t addr)
{

}

