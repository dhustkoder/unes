#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include <ogcsys.h>
#include <gccore.h>
#include "ppu.h"


static void render(const uint8_t* const nesfb)
{
	extern const uint32_t gc_nes_colors[0x40];
	extern uint32_t* gc_fb;
	extern GXRModeObj* gc_vmode;

	const long gc_width = gc_vmode->fbWidth;
	uint32_t* gc_fb_center = &gc_fb[((gc_width / 2) - NES_SCR_WIDTH) / 2];


	for (unsigned i = 0; i < NES_SCR_HEIGHT; ++i) {
		uint32_t* const vline = &gc_fb_center[i * gc_width];
		const uint8_t* const sline = &nesfb[i * NES_SCR_WIDTH];
		for (unsigned j = 0; j < NES_SCR_WIDTH; ++j)
			vline[j] = gc_nes_colors[sline[j]&0x3F];
	}
}


#endif
