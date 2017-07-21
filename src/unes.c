#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "platform.h"
#include "mmu.h"
#include "cpu.h"


static void runfor(const int_fast32_t clock_cycles)
{
	do {
		stepcpu();
		//stepapu();
		//stepppu();
	} while (get_cpu_clock() < clock_cycles);
	set_cpu_clock(get_cpu_clock() - clock_cycles);
}


int unes(const int argc, const char* const* argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!initmmu(argv[1]))
		return EXIT_FAILURE;

	resetcpu();

	uint_fast32_t time = gettime();
	int_fast8_t fps = 0;

	for (;;) {
		runfor(29830); // 1 frame
		//renderppu();
		if (++fps >= 60) {
			delay(1000 - (gettime() - time));
			time = gettime();
			fps = 0;
		}
	}

	termmmu();
	return EXIT_SUCCESS;
}

