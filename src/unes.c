#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "platform.h"
#include "mmu.h"
#include "cpu.h"


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
	for (;;) {
		stepcpu();
		if (get_cpu_clock() >= 1789773) {
			delay(1000 - (gettime() - time));
			time = gettime();
			set_cpu_clock(get_cpu_clock() - 1789773);
		}
	}

	termmmu();
	return EXIT_SUCCESS;
}

