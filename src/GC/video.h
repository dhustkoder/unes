#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include <time.h>
#include <gccore.h>
#include "log.h"


static void render(const uint8_t* const screen)
{
	extern const uint32_t gc_nes_rgb[0x40];
	extern uint32_t* gc_fb;
	extern GXRModeObj* gc_vmode;

	const uint16_t vwidth = gc_vmode->fbWidth;
	for (unsigned i = 0; i < 240; ++i) {
		uint32_t* restrict const vline = &gc_fb[i * vwidth + 28];
		const uint8_t* restrict const sline = &screen[i<<8];
		for (unsigned j = 0; j < 256; ++j)
			vline[j] = gc_nes_rgb[sline[j]];
	}

	VIDEO_Flush();
	VIDEO_WaitVSync();
}


#endif
