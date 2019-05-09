#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "log.h"
#include "ppu.h"


#ifdef PLATFORM_PS2
#define WIN_WIDTH      (320)
#define WIN_HEIGHT     (240)
#else
#define WIN_WIDTH      (NES_SCR_WIDTH * 3)
#define WIN_HEIGHT     (NES_SCR_HEIGHT * 3)
#endif


static inline void render(const uint8_t* const fb)
{
	extern SDL_Surface* sdl_surface;
	extern SDL_Color sdl_colors[0x40];

	#ifdef PLATFORM_PS2

	Uint32* pixels = sdl_surface->pixels;

	// center y
	pixels += ((WIN_HEIGHT / 2) - (NES_SCR_HEIGHT / 2)) * WIN_WIDTH;
	// center x 
	pixels += ((WIN_WIDTH / 2) - (NES_SCR_WIDTH / 2));

	for (int y = 0; y < NES_SCR_HEIGHT; ++y) {
		for (int x = 0; x < NES_SCR_WIDTH; ++x) {
			const int fbidx = y * NES_SCR_WIDTH + x;
			const int sfidx = y * WIN_WIDTH + x;
			const SDL_Color* const c = &sdl_colors[fb[fbidx]&0x3F];
			pixels[sfidx] = SDL_MapRGBA(sdl_surface->format, c->r, c->g, c->b, 0x00);
		}
	}

	#else
	#error Implement Render 
	#endif
}



#endif
