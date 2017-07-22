#include "ppu.h"

int_fast32_t ppuclk;

void resetppu(void)
{
	ppuclk = 0;
}

void stepppu(void)
{

}

void ppuwrite(uint_fast8_t val, int_fast32_t addr)
{

}

uint_fast8_t ppuread(int_fast32_t addr)
{
	return 0xFF;
}
