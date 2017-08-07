#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "SDL.h"


static inline void render(const uint32_t* const pixels, const uint_fast32_t len)
{
	extern SDL_Texture* const texture;
	extern SDL_Renderer* const renderer;

	int pitch;
	void* dest;
	SDL_LockTexture(texture, NULL, &dest, &pitch);
	memcpy(dest, pixels, len);
	SDL_UnlockTexture(texture);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}




#endif
