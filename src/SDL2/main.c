#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


SDL_AudioDeviceID audio_device;

static bool initsdl(void);
static void termsdl(void);
static void sighandler(int signum);
static bool signal_term = false;



static void runfor(const int_fast32_t cpu_clk_cycles)
{
	static int_fast32_t clk = 0;

	do {
		const int_fast8_t ticks = stepcpu();
		stepppu(ticks * 3);
		stepapu(ticks);
		//stepmapper(...)
		clk += ticks;
	} while (clk < cpu_clk_cycles);

	clk -= cpu_clk_cycles;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [rom]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!loadrom(argv[1]))
		return EXIT_FAILURE;

	int exitcode = EXIT_FAILURE;

	if (!initsdl())
		goto Lfreerom;


	signal(SIGINT, &sighandler);
	signal(SIGKILL, &sighandler);
	signal(SIGTERM, &sighandler);

	// resetmapper()
	resetcpu();
	resetapu();
	resetppu();

	while (!signal_term) {
		const Uint32 time = SDL_GetTicks();
		runfor(CPU_FREQ / 60);
		//renderppu();
		const Uint32 frametime = (SDL_GetTicks() - time);
		if (frametime < (1000 / 60))
			SDL_Delay((1000 / 60) - frametime);
	}

	extern const uint8_t rom_sram[0x2000];
	printf("SRAM:\n%s\n", &rom_sram[4]);

	exitcode = EXIT_SUCCESS;

//Ltermsdl:
	termsdl();
Lfreerom:
	freerom();
	return exitcode;
}

void sighandler(const int signum)
{
	signal_term = true;
	printf("\nsignal %d received.\n", signum);
}

bool initsdl(void)
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
		        SDL_GetError());
		return false;
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = 44100;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 2048;

	if ((audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0)) == 0) {
		fprintf(stderr, "Failed to open audio: %s", SDL_GetError());
		goto Lquitsdl;
	}

	SDL_PauseAudioDevice(audio_device, 0);
	return true;

Lquitsdl:
	SDL_Quit();
	return false;
}

void termsdl(void)
{
	SDL_CloseAudioDevice(audio_device);
	SDL_Quit();
}

