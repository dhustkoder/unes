#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


#define WIN_WIDTH  (256)
#define WIN_HEIGHT (240)


SDL_AudioDeviceID audio_device;
SDL_Texture* texture;
SDL_Renderer* renderer;
static SDL_Window* window;


static bool initsdl(void);
static void termsdl(void);
static bool update_events(void);


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

	// resetmapper()
	resetcpu();
	resetapu();
	resetppu();

	const int_fast32_t ticks = CPU_FREQ / 60;
	int_fast32_t clk = 0;

	while (update_events()) {
		do {
			const int_fast16_t step_ticks = stepcpu();
			stepppu(step_ticks * 3);
			stepapu(step_ticks);
			//stepmapper(...)
			clk += step_ticks;
		} while (clk < ticks);
		clk -= ticks;
	}

	exitcode = EXIT_SUCCESS;

//Ltermsdl:
	termsdl();
Lfreerom:
	freerom();
	return exitcode;
}


static bool update_events(void)
{
	static SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				return false;
			break;
		}
	}

	return true;
}


bool initsdl(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) != 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
		        SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow("µnes", SDL_WINDOWPOS_CENTERED,
				  SDL_WINDOWPOS_CENTERED,
				  WIN_WIDTH, WIN_HEIGHT,
				  SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		fprintf(stderr, "Failed to create SDL_Window: %s\n",
		        SDL_GetError());
		goto Lquitsdl;
	}

	renderer = SDL_CreateRenderer(window, -1,
	           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		fprintf(stderr, "Failed to create SDL_Renderer: %s\n",
		        SDL_GetError());
		goto Lfreewindow;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		                    SDL_TEXTUREACCESS_STREAMING,
		                    WIN_WIDTH, WIN_HEIGHT);
	if (texture == NULL) {
		fprintf(stderr, "Failed to create SDL_Texture: %s\n",
		        SDL_GetError());
		goto Lfreerenderer;
	}

	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = 44100;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 2048;

	if ((audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0)) == 0) {
		fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
		goto Lfreetexture;
	}

	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);
	SDL_PauseAudioDevice(audio_device, 0);
	return true;

Lfreetexture:
	SDL_DestroyTexture(texture);
Lfreerenderer:
	SDL_DestroyRenderer(renderer);
Lfreewindow:
	SDL_DestroyWindow(window);
Lquitsdl:
	SDL_Quit();
	return false;
}

void termsdl(void)
{
	SDL_CloseAudioDevice(audio_device);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

