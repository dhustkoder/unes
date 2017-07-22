#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "platform.h"
#include "mmu.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"

static void runfor(const int_fast32_t clock_cycles)
{
	extern int_fast32_t cpuclk, apuclk;

	do {
		stepcpu();
		stepapu();
		//stepppu();
	} while (cpuclk < clock_cycles);

	cpuclk = cpuclk - clock_cycles;
	apuclk = apuclk - clock_cycles;
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

	for (;;) {
		runfor(29830); // 1 frame
		//renderppu();
	}

	freerom();
	return EXIT_SUCCESS;
}

