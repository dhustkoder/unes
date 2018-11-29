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
	extern SDL_Texture* texture;
	extern const Uint32 nes_rgb[0x40];

	int pitch;
	Uint32* pixels;
	SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

	for (int i = 0; i < NES_SCR_WIDTH * NES_SCR_HEIGHT; ++i)
		pixels[i] = nes_rgb[fb[i]&0x3F];

	SDL_UnlockTexture(texture);
}


#endif
