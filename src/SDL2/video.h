#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "log.h"


static void render(const uint8_t* restrict const screen)
{
	extern SDL_Renderer* renderer;
	extern SDL_Texture* texture;
	extern Uint32 nes_rgb[0x40];

	static Uint32 frametimer = 0;

	const Uint32 now = SDL_GetTicks();
	const Uint32 timediff = now - frametimer;
	if (timediff >= (1000 / 60)) {
		int pitch;
		Uint32* pixels;
		SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

		for (unsigned i = 0; i < 240 * 256; ++i)
			pixels[i] = nes_rgb[screen[i]];

		SDL_UnlockTexture(texture);
		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		frametimer = now;
	}
}


#endif
