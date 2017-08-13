#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


#define TEXTURE_WIDTH  (256)
#define TEXTURE_HEIGHT (240)
#define WIN_WIDTH      TEXTURE_WIDTH
#define WIN_HEIGHT     TEXTURE_HEIGHT


SDL_AudioDeviceID audio_device;
SDL_Texture* texture;
SDL_Renderer* renderer;
static SDL_Window* window;


static const uint8_t keys_id[JOYPAD_NJOYPADS][KEY_NKEYS] = {
	[JOYPAD_ONE] = {
		[KEY_A]      = SDL_SCANCODE_Z,
		[KEY_B]      = SDL_SCANCODE_X,
		[KEY_SELECT] = SDL_SCANCODE_C,
		[KEY_START]  = SDL_SCANCODE_V,
		[KEY_UP]     = SDL_SCANCODE_UP,
		[KEY_DOWN]   = SDL_SCANCODE_DOWN,
		[KEY_LEFT]   = SDL_SCANCODE_LEFT,
		[KEY_RIGHT]  = SDL_SCANCODE_RIGHT
	},

	[JOYPAD_TWO] = {
		[KEY_A]      = SDL_SCANCODE_Q,
		[KEY_B]      = SDL_SCANCODE_E,
		[KEY_SELECT] = SDL_SCANCODE_R,
		[KEY_START]  = SDL_SCANCODE_T,
		[KEY_UP]     = SDL_SCANCODE_W,
		[KEY_DOWN]   = SDL_SCANCODE_S,
		[KEY_LEFT]   = SDL_SCANCODE_A,
		[KEY_RIGHT]  = SDL_SCANCODE_D
	}
};

uint8_t keys_state[JOYPAD_NJOYPADS][KEY_NKEYS];


static void update_key(const uint32_t code, const enum KeyState state)
{
	for (int pad = JOYPAD_ONE; pad < JOYPAD_NJOYPADS; ++pad) {
		for (int key = KEY_A; key < KEY_NKEYS; ++key) {
			if (keys_id[pad][key] == code) {
				keys_state[pad][key] = state;
				break;
			}
		}
	}
}

static bool update_events(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event) != 0) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		case SDL_KEYDOWN:
			if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
				return false;
			update_key(event.key.keysym.scancode, KEYSTATE_DOWN);
			break;
		case SDL_KEYUP:
			update_key(event.key.keysym.scancode, KEYSTATE_UP);
			break;
		}
	}

	return true;
}

static bool initsdl(void)
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
	           SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		fprintf(stderr, "Failed to create SDL_Renderer: %s\n",
		        SDL_GetError());
		goto Lfreewindow;
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		                    SDL_TEXTUREACCESS_STREAMING,
		                    TEXTURE_WIDTH, TEXTURE_HEIGHT);
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

	for (int pad = JOYPAD_ONE; pad < JOYPAD_NJOYPADS; ++pad)
		for (int key = KEY_A; key < KEY_NKEYS; ++key)
			keys_state[pad][key] = KEYSTATE_UP;

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

static void termsdl(void)
{
	SDL_CloseAudioDevice(audio_device);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}


int main(const int argc, const char* const* const argv)
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

	resetcpu();
	resetapu();
	resetppu();

	const int_fast32_t frameticks = CPU_FREQ / 60;
	int_fast32_t clk = 0;

	while (update_events()) {
		do {
			const int_fast16_t ticks = stepcpu();
			stepppu(ticks * 3);
			stepapu(ticks);
			clk += ticks;
		} while (clk < frameticks);
		clk -= frameticks;
	}

	exitcode = EXIT_SUCCESS;

//Ltermsdl:
	termsdl();
Lfreerom:
	freerom();
	return exitcode;
}

