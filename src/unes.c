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

	uint_fast32_t time = gettime();
	int_fast8_t fps = 0;

	for (;;) {
		runfor(29830); // run for 1 frame
		//renderppu();
		if (++fps >= 60) {
			delay(1000 - (gettime() - time));
			time = gettime();
			fps = 0;
		}
	}

	freerom();
	return EXIT_SUCCESS;
}

