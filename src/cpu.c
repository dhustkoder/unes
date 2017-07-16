#include "types.h"
#include "mem.h"
#include "cpu.h"


static uint_fast16_t pc;


void initcpu(void)
{
	pc = memread16(ADDR_RESET_VECTOR);
}


void stepcpu(void)
{
	
}

