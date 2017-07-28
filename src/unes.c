#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include "platform.h"
#include "mmu.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


static bool signal_term = false;

static void sighandler(const int signum)
{
	signal_term = true;
	printf("\nsignal %d received.\n", signum);
}


static void runfor(const int_fast32_t cpu_clk_cycles)
{
	static int_fast32_t clk = 0;

	do {
		const int_fast8_t ticks = stepcpu();
		stepppu(ticks * 3);
		stepapu(ticks);
		clk += ticks;
	} while (clk < cpu_clk_cycles);

	clk -= cpu_clk_cycles;
}


int unes(const int argc, const char* const* argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!loadrom(argv[1]))
		return EXIT_FAILURE;

	signal(SIGINT, &sighandler);
	signal(SIGKILL, &sighandler);
	signal(SIGTERM, &sighandler);

	resetcpu();
	resetapu();
	resetppu();
	
	uint_fast32_t time = gettime();
	int_fast8_t fps = 0;
	while (!signal_term) {
		runfor(CPU_FREQ / 60); // run for 1 frame
		//renderppu();
		if (++fps > 60) {
			delay(1000 - (gettime() - time));
			fps = 0;
			time = gettime();
		}
	}

	extern const uint8_t mmu_test_text[0x2000];
	printf("TEST REPORT: %s\n", &mmu_test_text[4]);

	freerom();
	return EXIT_SUCCESS;
}

