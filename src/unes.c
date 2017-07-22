#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "platform.h"
#include "mmu.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


static void runfor(const int_fast32_t clock_cycles)
{
	extern int_fast32_t cpuclk, apuclk, ppuclk;

	do {
		stepcpu();
		stepapu();
		stepppu();
	} while (cpuclk < clock_cycles);

	cpuclk -= clock_cycles;
	apuclk -= clock_cycles;
	ppuclk -= clock_cycles;
}


int unes(const int argc, const char* const* argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!loadrom(argv[1]))
		return EXIT_FAILURE;


	resetcpu();
	resetapu();
	resetppu();
	
	uint_fast32_t time = gettime();

	for (;;) {
		runfor(CPU_FREQ); // run for 1 second
		//renderppu();
		delay(1000 - (gettime() - time));
		time = gettime();
	}

	freerom();
	return EXIT_SUCCESS;
}

