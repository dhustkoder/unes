#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "log.h"
#include "ppu.h"


#define TEXTURE_WIDTH  (NES_SCR_WIDTH)
#define TEXTURE_HEIGHT (NES_SCR_HEIGHT)
#ifdef PLATFORM_PS2
#define WIN_WIDTH      (640)
#define WIN_HEIGHT     (400)
#else
#define WIN_WIDTH      (NES_SCR_WIDTH * 3)
#define WIN_HEIGHT     (NES_SCR_HEIGHT * 3)
#endif


static inline void render(const uint8_t* const fb)
{
	extern SDL_Surface* sdl_surface;
	extern const SDL_Color sdl_colors[0x40];

    SDL_LockSurface(sdl_surface);
   	
   	// center on screen, PS2 tests
   	Uint32* pixdata = sdl_surface->pixels;
    pixdata += ((WIN_WIDTH / 2) - (TEXTURE_WIDTH / 2));
    pixdata += WIN_WIDTH * ((WIN_HEIGHT / 2) - (TEXTURE_HEIGHT / 2));

    for (int y = 0; y < TEXTURE_HEIGHT; ++y) {
    	for (int x = 0; x < TEXTURE_WIDTH; ++x) {
    		const SDL_Color* const c = &sdl_colors[fb[y*TEXTURE_WIDTH + x]];
    		pixdata[x] = SDL_MapRGB(sdl_surface->format, c->r, c->g, c->b);
    	}
    	pixdata += sdl_surface->w;
    }

    SDL_UnlockSurface(sdl_surface);

}


#endif
