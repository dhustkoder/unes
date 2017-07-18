#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
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

	for (;;)
		stepcpu();

	termmmu();
	return EXIT_SUCCESS;
}

