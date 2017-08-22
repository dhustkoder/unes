#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include "ninjaslapper_bin.h"
#include "instrtest_bin.h"
#include "log.h"
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"


#define ROMDATA ninjaslapper

const uint32_t gc_nes_rgb[0x40] = {
	COLOR_GRAY, COLOR_BLUE, COLOR_BLUE, COLOR_PURPLE, COLOR_RED, COLOR_RED,
	COLOR_RED, COLOR_MAROON, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_BLUE,
	COLOR_BLACK, COLOR_BLACK, COLOR_BLACK, COLOR_SILVER, COLOR_BLUE, COLOR_BLUE,
	COLOR_PURPLE, COLOR_PURPLE, COLOR_RED, COLOR_RED, COLOR_YELLOW, COLOR_YELLOW,
	COLOR_GREEN, COLOR_GREEN, COLOR_GREEN, COLOR_BLUE, COLOR_BLACK, COLOR_BLACK,
	COLOR_BLACK, COLOR_WHITE, COLOR_BLUE, COLOR_PURPLE, COLOR_PURPLE, COLOR_PURPLE,
	COLOR_RED, COLOR_YELLOW, COLOR_YELLOW, COLOR_YELLOW, COLOR_GREEN, COLOR_GREEN,
	COLOR_GREEN, COLOR_BLUE, COLOR_GRAY, COLOR_BLACK, COLOR_BLACK, COLOR_WHITE,
	COLOR_BLUE, COLOR_PURPLE, COLOR_PURPLE, COLOR_RED, COLOR_RED, COLOR_YELLOW,
	COLOR_YELLOW, COLOR_YELLOW, COLOR_GREEN, COLOR_GREEN, COLOR_BLUE, COLOR_WHITE,
	COLOR_BLACK, COLOR_BLACK
};

uint8_t gc_padstate[JOYPAD_NJOYPADS];

GXRModeObj* gc_vmode;
uint32_t* gc_fb;
static void* console_fb;


static void initialize_platform(void)
{
	VIDEO_Init();
	PAD_Init();
	
	gc_vmode = VIDEO_GetPreferredMode(NULL);
	VIDEO_Configure(gc_vmode);

	gc_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gc_vmode));
	console_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gc_vmode));
	
	console_init(console_fb, 20, 20, gc_vmode->fbWidth, gc_vmode->xfbHeight,
	             gc_vmode->fbWidth * 2);

	VIDEO_ClearFrameBuffer(gc_vmode, gc_fb, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(gc_fb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (gc_vmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}

static void quit(void)
{
	for (;;)
		VIDEO_WaitVSync();
}


void main(void)
{
	initialize_platform();

	if (!loadrom(ROMDATA)) {
		logerror("Couldn't load rom!\n");
		quit();
	}

	resetcpu();
	resetppu();
	resetapu();

	const uint32_t frameclks = CPU_FREQ / 30;
	uint32_t clk = 0;

	for (;;) {
		do {
			const unsigned stepclks = stepcpu();
			stepppu(stepclks * 3);
			stepapu(stepclks);
			clk += stepclks;
		} while (clk < frameclks);
		clk -= frameclks;

		PAD_ScanPads();
		for (unsigned i = 0; i < JOYPAD_NJOYPADS; ++i) {
			const int buttons = PAD_ButtonsHeld(i);

			if (buttons&PAD_BUTTON_Y)
				VIDEO_SetNextFramebuffer(console_fb);
			if (buttons&PAD_BUTTON_X)
				VIDEO_SetNextFramebuffer(gc_fb);

			gc_padstate[i] =
			  ((buttons&PAD_BUTTON_A) != 0)<<KEY_A          |
			  ((buttons&PAD_BUTTON_B) != 0)<<KEY_B          |
			  ((buttons&PAD_TRIGGER_Z) != 0)<<KEY_SELECT    |
			  ((buttons&PAD_BUTTON_START) != 0)<<KEY_START  |
			  ((buttons&PAD_BUTTON_LEFT) != 0)<<KEY_LEFT    |
			  ((buttons&PAD_BUTTON_RIGHT) != 0)<<KEY_RIGHT  |
			  ((buttons&PAD_BUTTON_UP) != 0)<<KEY_UP        |
			  ((buttons&PAD_BUTTON_DOWN) != 0)<<KEY_DOWN;
		}
	}
}
