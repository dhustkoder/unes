#ifndef UNES_VIDEO_H_
#define UNES_VIDEO_H_
#include <stdint.h>
#include <gccore.h>


static void render(const uint8_t* const screen)
{
	extern const uint32_t gc_nes_colors[0x40];
	extern uint32_t* gc_fb;

	for (unsigned i = 0; i < 240; ++i) {
		uint32_t* restrict const vline = &gc_fb[i * 640 + 28];
		const uint8_t* restrict const sline = &screen[i<<8];
		for (unsigned j = 0; j < 256; ++j)
			vline[j] = gc_nes_colors[sline[j]];
		memcpy(vline + 320, vline, sizeof(uint32_t) * 256);
	}
}


#endif
