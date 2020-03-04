#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include <gsKit.h>
#include <dmaKit.h>
#include "log.h"
#include "ppu.h"


#define WIN_WIDTH      (NES_SCR_WIDTH)
#define WIN_HEIGHT     (NES_SCR_HEIGHT)




static inline void render(const u8* const palette_idxs)
{
	extern const u32 nes_rgb_palette[0x40];
	
	extern GSTEXTURE fb;

	int stride = fb.Width;
	u32* pixels = (void*)fb.Mem;

	for (int y = 0; y < WIN_HEIGHT; ++y) {
		for (int x = 0; x < WIN_WIDTH; ++x) {
			pixels[y * stride + x] = nes_rgb_palette[palette_idxs[y * WIN_WIDTH + x]&0x3F];
		}
	}


}



#endif
