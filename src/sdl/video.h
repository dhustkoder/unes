#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "log.h"
#include "ppu.h"


#define TEXTURE_WIDTH  (NES_SCR_WIDTH)
#define TEXTURE_HEIGHT (NES_SCR_HEIGHT)
#define WIN_WIDTH      (NES_SCR_WIDTH * 3)
#define WIN_HEIGHT     (NES_SCR_HEIGHT * 3)

static inline void render(const uint8_t* const fb)
{
	extern SDL_Surface* sdl_surface;

    SDL_LockSurface( sdl_surface );

    memcpy(sdl_surface->pixels, fb, TEXTURE_WIDTH * TEXTURE_HEIGHT);

    SDL_UnlockSurface( sdl_surface );

}


#endif
