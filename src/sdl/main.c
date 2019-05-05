#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "SDL.h"
#include "audio.h"
#include "video.h"
#include "log.h"
#include "rom.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"


const Uint32 sdl_nes_rgb[0x40] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

SDL_Color sdl_colors[0x40] = {
	{.r = 0x7C, .g = 0x7C, .b = 0x7C }, {.r = 0x00, .g = 0x00, .b = 0xFC }, {.r = 0x00, .g = 0x00, .b = 0xBC }, 
	{.r = 0x44, .g = 0x28, .b = 0xBC }, {.r = 0x94, .g = 0x00, .b = 0x84 }, {.r = 0xA8, .g = 0x00, .b = 0x20 },
	{.r = 0xA8, .g = 0x10, .b = 0x00 }, {.r = 0x88, .g = 0x14, .b = 0x00 }, {.r = 0x50, .g = 0x30, .b = 0x00 }, 
	{.r = 0x00, .g = 0x78, .b = 0x00 }, {.r = 0x00, .g = 0x68, .b = 0x00 }, {.r = 0x00, .g = 0x58, .b = 0x00 },
	{.r = 0x00, .g = 0x40, .b = 0x58 }, {.r = 0x00, .g = 0x00, .b = 0x00 }, {.r = 0x00, .g = 0x00, .b = 0x00 },
	{.r = 0x00, .g = 0x00, .b = 0x00 }, {.r = 0xBC, .g = 0xBC, .b = 0xBC }, {.r = 0x00, .g = 0x78, .b = 0xF8 },
	{.r = 0x00, .g = 0x58, .b = 0xF8 }, {.r = 0x68, .g = 0x44, .b = 0xFC }, {.r = 0xD8, .g = 0x00, .b = 0xCC },
	{.r = 0xE4, .g = 0x00, .b = 0x58 }, {.r = 0xF8, .g = 0x38, .b = 0x00 }, {.r = 0xE4, .g = 0x5C, .b = 0x10 },
	{.r = 0xAC, .g = 0x7C, .b = 0x00 }, {.r = 0x00, .g = 0xB8, .b = 0x00 }, {.r = 0x00, .g = 0xA8, .b = 0x00 },
	{.r = 0x00, .g = 0xA8, .b = 0x44 }, {.r = 0x00, .g = 0x88, .b = 0x88 }, {.r = 0x00, .g = 0x00, .b = 0x00 },
	{.r = 0x00, .g = 0x00, .b = 0x00 }, {.r = 0x00, .g = 0x00, .b = 0x00 }, {.r = 0xF8, .g = 0xF8, .b = 0xF8 },
	{.r = 0x3C, .g = 0xBC, .b = 0xFC }, {.r = 0x68, .g = 0x88, .b = 0xFC }, {.r = 0x98, .g = 0x78, .b = 0xF8 }, 
	{.r = 0xF8, .g = 0x78, .b = 0xF8 }, {.r = 0xF8, .g = 0x58, .b = 0x98 }, {.r = 0xF8, .g = 0x78, .b = 0x58 },
	{.r = 0xFC, .g = 0xA0, .b = 0x44 }, {.r = 0xF8, .g = 0xB8, .b = 0x00 }, {.r = 0xB8, .g = 0xF8, .b = 0x18 }, 
	{.r = 0x58, .g = 0xD8, .b = 0x54 }, {.r = 0x58, .g = 0xF8, .b = 0x98 }, {.r = 0x00, .g = 0xE8, .b = 0xD8 }, 
	{.r = 0x78, .g = 0x78, .b = 0x78 }, {.r = 0x00, .g = 0x00, .b = 0x00 }, {.r = 0x00, .g = 0x00, .b = 0x00 },
	{.r = 0xFC, .g = 0xFC, .b = 0xFC }, {.r = 0xA4, .g = 0xE4, .b = 0xFC }, {.r = 0xB8, .g = 0xB8, .b = 0xF8 },
	{.r = 0xD8, .g = 0xB8, .b = 0xF8 }, {.r = 0xF8, .g = 0xB8, .b = 0xF8 }, {.r = 0xF8, .g = 0xA4, .b = 0xC0 },
	{.r = 0xF0, .g = 0xD0, .b = 0xB0 }, {.r = 0xFC, .g = 0xE0, .b = 0xA8 }, {.r = 0xF8, .g = 0xD8, .b = 0x78 },
	{.r = 0xD8, .g = 0xF8, .b = 0x78 }, {.r = 0xB8, .g = 0xF8, .b = 0xB8 }, {.r = 0xB8, .g = 0xF8, .b = 0xD8 }, 
	{.r = 0x00, .g = 0xFC, .b = 0xFC }, {.r = 0xF8, .g = 0xD8, .b = 0xF8 }, {.r = 0x00, .g = 0x00, .b = 0x00 }, 
	{.r = 0x00, .g=  0x00, .b = 0x00 }
};

Uint8 sdl_padstate[2] = { 
	[JOYPAD_ONE] = KEYSTATE_UP,
	[JOYPAD_TWO] = KEYSTATE_UP 
};

SDL_Surface* sdl_surface;

static const Uint8 keys_id[2][8] = {
	[JOYPAD_ONE] = {
		[KEY_A]      = SDLK_z,
		[KEY_B]      = SDLK_x,
		[KEY_SELECT] = SDLK_c,
		[KEY_START]  = SDLK_v,
		[KEY_UP]     = SDLK_UP,
		[KEY_DOWN]   = SDLK_DOWN,
		[KEY_LEFT]   = SDLK_LEFT,
		[KEY_RIGHT]  = SDLK_RIGHT
	},

	[JOYPAD_TWO] = {
		[KEY_A]      = SDLK_q,
		[KEY_B]      = SDLK_e,
		[KEY_SELECT] = SDLK_r,
		[KEY_START]  = SDLK_t,
		[KEY_UP]     = SDLK_w,
		[KEY_DOWN]   = SDLK_s,
		[KEY_LEFT]   = SDLK_a,
		[KEY_RIGHT]  = SDLK_d
	}
};


static void update_key(const Uint32 code, const key_state_t state)
{
	for (unsigned pad = 0; pad < 2; ++pad) {
		for (unsigned key = 0; key < 8; ++key) {
			if (keys_id[pad][key] == code) {
				sdl_padstate[pad] &= ~(0x01<<key);
				sdl_padstate[pad] |= state<<key;
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
			if (event.key.keysym.sym == SDLK_ESCAPE)
				return false;
			update_key(event.key.keysym.sym, KEYSTATE_DOWN);
			break;
		case SDL_KEYUP:
			update_key(event.key.keysym.sym, KEYSTATE_UP);
			break;
		}
	}

	return true;
}

static bool initialize_platform(void)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		log_error("Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	sdl_surface = SDL_SetVideoMode(TEXTURE_WIDTH, TEXTURE_HEIGHT, 8, SDL_SWSURFACE);
	if (sdl_surface == NULL) {
		log_error("Couldn't initialize Surface: %s\n", SDL_GetError());
		goto Lquitsdl;
	}

	SDL_SetColors(sdl_surface, sdl_colors, 0, 0x40);

	return true;
Lquitsdl:
	SDL_Quit();
	return false;
}

static void terminate_platform(void)
{
	SDL_Quit();
}

static uint8_t* read_file(const char* const filepath)
{
	FILE* const file = fopen(filepath, "r");
	if (file == NULL) {
		log_error("Couldn't open \'%s\': %s\n", filepath, strerror(errno));
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	const size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* data = malloc(size);
	if (data == NULL) {
		log_error("Couldn't allocate memory: %s\n", strerror(errno));
		goto Lfclose;
	}

	if (fread(data, 1, size, file) < size && ferror(file) != 0) {
		log_error("Couldn't read \'%s\': %s\n", filepath, strerror(errno));
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
		log_error("Usage: %s [rom]\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!initialize_platform())
		return EXIT_FAILURE;

	int exitcode = EXIT_FAILURE;

	const uint8_t* const rom = read_file(argv[1]);
	if (rom == NULL)
		goto Lterminate_platform;

	if (!rom_load(rom))
		goto Lfreerom;

	cpu_reset();
	apu_reset();
	ppu_reset();

	const Sint32 ticks_per_sec = NES_CPU_FREQ / 60;
	Sint32 ticks = 0;
	
	while (update_events()) {

		do {
			const short step_ticks = cpu_step();
			ppu_step((step_ticks<<1) + step_ticks);
			apu_step(step_ticks);
			ticks += step_ticks;
		} while (ticks < ticks_per_sec);

		ticks -= ticks_per_sec;

		SDL_Flip(sdl_surface);

		#ifdef UNES_LOG_STATE
		cpu_log_state();
		ppu_log_state();
		#endif
	}

	exitcode = EXIT_SUCCESS;
	rom_unload();
Lfreerom:
	free((void*)rom);
Lterminate_platform:
	terminate_platform();
	return exitcode;
}
