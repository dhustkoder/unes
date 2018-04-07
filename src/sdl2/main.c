#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "SDL.h"
#include "audio.h"
#include "log.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


#define TEXTURE_WIDTH  (256)
#define TEXTURE_HEIGHT (240)
#define WIN_WIDTH      TEXTURE_WIDTH
#define WIN_HEIGHT     TEXTURE_HEIGHT


const Uint32 nes_rgb[0x40] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};


Uint8 sdl2_padstate[2] = { 
	[JOYPAD_ONE] = KEYSTATE_UP,
	[JOYPAD_TWO] = KEYSTATE_UP 
};

SDL_AudioDeviceID audio_device;
SDL_Renderer* renderer;
SDL_Texture* texture;
static SDL_Window* window;

static const Uint8 keys_id[2][8] = {
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


static void update_key(const Uint32 code, const key_state_t state)
{
	for (unsigned pad = 0; pad < 2; ++pad) {
		for (unsigned key = 0; key < 8; ++key) {
			if (keys_id[pad][key] == code) {
				sdl2_padstate[pad] &= ~(0x01<<key);
				sdl2_padstate[pad] |= state<<key;
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

static bool initialize_platform(void)
{
	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) != 0) {
		logerror("Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	// video
	window = SDL_CreateWindow("µnes", SDL_WINDOWPOS_CENTERED,
				  SDL_WINDOWPOS_CENTERED,
				  WIN_WIDTH, WIN_HEIGHT,
				  SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		logerror("Failed to create SDL_Window: %s\n", SDL_GetError());
		goto Lquitsdl;
	}

	renderer = SDL_CreateRenderer(window, -1,
	                              SDL_RENDERER_ACCELERATED|
	                              SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		logerror("Failed to create SDL_Renderer: %s\n", SDL_GetError());
		goto Lfreewindow;
	}

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	texture = SDL_CreateTexture(renderer, info.texture_formats[0],
	                            SDL_TEXTUREACCESS_STREAMING,
				    TEXTURE_WIDTH, TEXTURE_HEIGHT);
	if (texture == NULL) {
		logerror("Failed to create SDL_Texture: %s\n", SDL_GetError());
		goto Lfreerenderer;
	}

	// audio
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = AUDIO_FREQUENCY;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = AUDIO_BUFFER_SIZE;
	if ((audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0)) == 0) {
		logerror("Failed to open audio: %s\n", SDL_GetError());
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

static void terminate_platform(void)
{
	SDL_CloseAudioDevice(audio_device);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

static uint8_t* readfile(const char* filepath)
{
	FILE* const file = fopen(filepath, "r");
	if (file == NULL) {
		logerror("Couldn't open \'%s\'\n", filepath);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	const size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* data = malloc(size);
	if (data == NULL) {
		logerror("Couldn't allocate memory\n");
		goto Lfclose;
	}

	if (fread(data, 1, size, file) < size && ferror(file) != 0) {
		logerror("Couldn't read \'%s\'\n", filepath);
		free(data);
		data = NULL;
		goto Lfclose;
	}

Lfclose:
	fclose(file);
	return data;
}


int main(int argc, char* argv[])
{
	if (argc < 2) {
		logerror("Usage: %s [rom]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!initialize_platform())
		return EXIT_FAILURE;

	int exitcode = EXIT_FAILURE;

	uint8_t* const rom = readfile(argv[1]);
	if (rom == NULL)
		goto Lterminate_platform;

	if (!loadrom(rom))
		goto Lfreerom;

	resetcpu();
	resetapu();
	resetppu();

	//#define UNES_SDL2_FPS_BENCH

	#ifdef UNES_SDL2_FPS_BENCH
	Uint32 fpstimer = SDL_GetTicks();
	int fps = 0;
	#endif

	const int32_t frameclk = NES_CPU_FREQ / 60;
	int32_t clk = 0;
	while (update_events()) {
		do {
			const unsigned ticks = stepcpu();
			stepppu((ticks<<1) + ticks);
			stepapu(ticks);
			clk += ticks;
		} while (clk < frameclk);
		clk -= frameclk;

		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		#ifdef UNES_SDL2_FPS_BENCH
		++fps;
		const Uint32 fpsnow = SDL_GetTicks();
		if ((fpsnow - fpstimer) >= 1000) {
			loginfo("%d\n", fps);
			fps = 0;
			fpstimer = fpsnow;
		}
		#endif

		log_ppu_state();
	}

	exitcode = EXIT_SUCCESS;
	unloadrom();
Lfreerom:
	free(rom);
Lterminate_platform:
	terminate_platform();
	return exitcode;
}