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


static void runfor(const int_fast32_t cpu_clk_cycles)
{
	extern int_fast32_t cpuclk;

	do {
		stepcpu();
		stepapu();
		stepppu();
	} while (cpuclk < cpu_clk_cycles);

	cpuclk -= cpu_clk_cycles;
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
	int_fast8_t fps = 0;
	for (;;) {
		runfor(CPU_FREQ / 60); // run for 1 frame
		//renderppu();
		if (++fps > 60) {
			delay(1000 - (gettime() - time));
			fps = 0;
			time = gettime();
		}
	}

	freerom();
	return EXIT_SUCCESS;
}

