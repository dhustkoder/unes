#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include "ppu.h"

#define TEXTURE_WIDTH  (NES_SCR_WIDTH)
#define TEXTURE_HEIGHT (NES_SCR_HEIGHT)
#define WIN_WIDTH      (NES_SCR_WIDTH * 3)
#define WIN_HEIGHT     (NES_SCR_HEIGHT * 3)

static inline void render(const uint8_t* const fb)
{
	extern void video_internal_render(const uint8_t* const fb);
	video_internal_render(fb);
}


#endif
