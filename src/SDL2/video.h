#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"
#include "SDL_opengl.h"


#define FPS_LIMIT (59)


static void render(const uint8_t* restrict const screen)
{
	extern SDL_Renderer* renderer;
	extern SDL_Texture* texture;
	extern Uint32 nes_rgb[0x40];

	static Uint32 frametimer = 0;
	static Uint32 fpstimer = 0;

	int pitch;
	Uint32* pixels;
	SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

	for (unsigned i = 0u; i < 240u * 256u; ++i)
		pixels[i] = nes_rgb[screen[i]];

	SDL_UnlockTexture(texture);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);

	static unsigned fps = 0;
	++fps;
	if ((SDL_GetTicks() - fpstimer) >= 1000) {
		printf("%u\n", fps);
		fps = 0;
		fpstimer = SDL_GetTicks();
	}
	
	const Uint32 timediff = SDL_GetTicks() - frametimer;
	if (timediff < (1000 / FPS_LIMIT))
		SDL_Delay((1000 / FPS_LIMIT) - timediff);
	frametimer = SDL_GetTicks();
}


#endif
