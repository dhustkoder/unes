#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include <time.h>
#include <gccore.h>
#include "log.h"


static void render(const uint8_t* restrict const screen)
{
	extern const uint32_t gc_nes_rgb[0x40];
	extern uint32_t* gc_fb;
	extern GXRModeObj* gc_vmode;

	for (unsigned i = 0; i < 240; ++i) {
		for (unsigned j = 0; j < 256; ++j) {
			gc_fb[i * gc_vmode->fbWidth + 28 + j] = 
			    gc_nes_rgb[screen[i * 256 + j]];
		}
	}

	VIDEO_Flush();
	VIDEO_WaitVSync();
}


#endif
