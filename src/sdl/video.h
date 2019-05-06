#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "log.h"
#include "ppu.h"


#define TEXTURE_WIDTH  (NES_SCR_WIDTH)
#define TEXTURE_HEIGHT (NES_SCR_HEIGHT)
#ifdef PLATFORM_PS2
#define WIN_WIDTH      (TEXTURE_WIDTH)
#define WIN_HEIGHT     (TEXTURE_HEIGHT)
#else
#define WIN_WIDTH      (NES_SCR_WIDTH * 3)
#define WIN_HEIGHT     (NES_SCR_HEIGHT * 3)
#endif


static inline void render(const uint8_t* const fb)
{
	extern SDL_Surface* sdl_fb;
	extern const Uint32 sdl_nes_rgb[0x40];
	SDL_LockSurface(sdl_fb);
	memcpy(sdl_fb->pixels, fb, TEXTURE_WIDTH * TEXTURE_HEIGHT);
	SDL_UnlockSurface(sdl_fb);
}



#endif
