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
#ifdef PLATFORM_PS2
#include "rom_data.h"
#endif


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

#ifdef PLATFORM_PS2
#define SQUARE_IDX   (0x00)
#define CROSS_IDX    (0x01)
#define SELECT_IDX   (0x04)
#define START_IDX    (0x05)
static SDL_Joystick* sdl_joystick;

static void update_button(const joypad_t pad,
                          const uint8_t button,
                          const key_state_t state)
{
	uint8_t key = 0;
	switch (button) {
		case SQUARE_IDX: key = KEY_A; break;
		case CROSS_IDX: key = KEY_B; break;
		case SELECT_IDX: key = KEY_SELECT; break;
		case START_IDX: key = KEY_START; break;
		default: return;
	}
	sdl_padstate[pad] &= ~(0x01<<key);
	sdl_padstate[pad] |= state<<key;
}

static void update_hat(const joypad_t pad,
                       const uint8_t value)
{
	sdl_padstate[pad] &= ~(1<<KEY_UP|1<<KEY_DOWN|1<<KEY_LEFT|1<<KEY_RIGHT);
	switch (value) {
		case SDL_HAT_UP: sdl_padstate[pad] |= 1<<KEY_UP; break;
		case SDL_HAT_DOWN: sdl_padstate[pad] |= 1<<KEY_DOWN; break;
		case SDL_HAT_LEFT: sdl_padstate[pad] |= 1<<KEY_LEFT; break;
		case SDL_HAT_RIGHT: sdl_padstate[pad] |= 1<<KEY_RIGHT; break;
		case SDL_HAT_RIGHTUP: sdl_padstate[pad] |= 1<<KEY_RIGHT|1<<KEY_UP; break;
		case SDL_HAT_RIGHTDOWN: sdl_padstate[pad] |= 1<<KEY_RIGHT|1<<KEY_DOWN; break;
		case SDL_HAT_LEFTUP: sdl_padstate[pad] |= 1<<KEY_LEFT|1<<KEY_UP; break;
		case SDL_HAT_LEFTDOWN: sdl_padstate[pad] |= 1<<KEY_LEFT|1<<KEY_DOWN; break;
	}
}

#else
static const unsigned keys_id[2][8] = {
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


#endif


static bool update_events(void)
{
	SDL_Event event;

	while (SDL_PollEvent(&event) != 0) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		#ifdef PLATFORM_PS2
		case SDL_JOYHATMOTION:
			update_hat(event.jhat.which, event.jhat.value);
			break;
		case SDL_JOYBUTTONDOWN:
			update_button(event.jbutton.which, event.jbutton.button, KEYSTATE_DOWN);
			break;
		case SDL_JOYBUTTONUP:
			update_button(event.jbutton.which, event.jbutton.button, KEYSTATE_UP);
			break;
		#else
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_ESCAPE)
				return false;
			update_key(event.key.keysym.sym, KEYSTATE_DOWN);
			break;
		case SDL_KEYUP:
			update_key(event.key.keysym.sym, KEYSTATE_UP);
			break;
		#endif
		}
	}

	return true;
}

static bool initialize_platform(void)
{
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_JOYSTICK) != 0) {
		log_error("Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	sdl_surface = SDL_SetVideoMode(WIN_WIDTH, WIN_HEIGHT, 32,
	                               SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_RESIZABLE);
	if (sdl_surface == NULL) {
		log_error("Couldn't initialize Surface: %s\n", SDL_GetError());
		goto Lquitsdl;
	}

	#ifdef PLATFORM_PS2
	sdl_joystick = SDL_JoystickOpen(0);
	if (sdl_joystick == NULL) {
		log_error("Couldn't open joystick index 0: %s\n", SDL_GetError());
		goto Lfree_surface;
	}
	#endif

	SDL_ShowCursor(SDL_DISABLE);

	return true;

#ifdef PLATFORM_PS2
Lfree_surface:
	SDL_FreeSurface(sdl_surface);
#endif
Lquitsdl:
	SDL_Quit();
	return false;
}

static void terminate_platform(void)
{
	#ifdef PLATFORM_PS2
	SDL_JoystickClose(sdl_joystick);
	#endif
	SDL_FreeSurface(sdl_surface);
	SDL_Quit();
}

#ifndef PLATFORM_PS2
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
#endif


int main(int argc, char* argv[])
{
	#ifndef PLATFORM_PS2
	
	if (argc < 2) {
		log_error("Usage: %s [rom]\n", argv[0]);
		return EXIT_FAILURE;
	}

	#endif

	
	if (!initialize_platform())
		return EXIT_FAILURE;

	int exitcode = EXIT_FAILURE;

	#ifndef PLATFORM_PS2
	const uint8_t* const rom = read_file(argv[1]);
	#else
	const uint8_t* const rom = rom_data;
	#endif

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
		SDL_LockSurface(sdl_surface);
		do {
			const short step_ticks = cpu_step();
			ppu_step((step_ticks<<1) + step_ticks);
			apu_step(step_ticks);
			ticks += step_ticks;
		} while (ticks < ticks_per_sec);
		SDL_UnlockSurface(sdl_surface);
		ticks -= ticks_per_sec;

		SDL_Flip(sdl_surface);
		SDL_Delay(1);

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
