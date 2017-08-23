#include <time.h>
#include <ogcsys.h>
#include <gccore.h>
#include <inttypes.h>
#include "log.h"
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "ninjaslapper_bin.h"
#include "instrtest_bin.h"
#include "dushlan_bin.h"
#include "megamanii_bin.h"
#include "donkeykong_bin.h"
#include "tetris_bin.h"


const uint32_t gc_nes_colors[0x40] = {
	COLOR_GRAY, COLOR_BLUE, COLOR_BLUE, COLOR_PURPLE, COLOR_RED, COLOR_RED,
	COLOR_RED, COLOR_RED, COLOR_MAROON, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
	COLOR_BLUE, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK,

	COLOR_GRAY, COLOR_BLUE, COLOR_BLUE, COLOR_PURPLE, COLOR_RED, COLOR_RED,
	COLOR_RED, COLOR_RED, COLOR_MAROON, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
	COLOR_BLUE, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK,

	COLOR_WHITE, COLOR_BLUE, COLOR_BLUE, COLOR_PURPLE, COLOR_RED, COLOR_RED,
	COLOR_RED, COLOR_RED, COLOR_MAROON, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
	COLOR_BLUE, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK,

	COLOR_WHITE, COLOR_BLUE, COLOR_BLUE, COLOR_PURPLE, COLOR_RED, COLOR_RED,
	COLOR_RED, COLOR_RED, COLOR_MAROON, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN,
	COLOR_BLUE, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK
};

uint8_t gc_padstate[JOYPAD_NJOYPADS];
uint32_t* gc_fb;
static void* console_fb;


static void initialize_platform(void)
{
	VIDEO_Init();
	PAD_Init();
	
	GXRModeObj* const vmode = VIDEO_GetPreferredMode(NULL);
	VIDEO_Configure(vmode);

	gc_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));
	console_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(vmode));

	console_init(console_fb, 0, 0, vmode->fbWidth, vmode->xfbHeight,
	             vmode->fbWidth * 2);

	VIDEO_ClearFrameBuffer(vmode, gc_fb, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(gc_fb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (vmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	loginfo("GC VMODE INFO:\n"
		"viTVMode: %" PRIu32 "\n"
	        "fbWidth: %" PRIu16 "\n"
		"efbHeight: %" PRIu16 "\n"
		"xfbHeight: %" PRIu16 "\n"
		"viXOrigin: %" PRIu16 "\n"
		"viYOrigin: %" PRIu16 "\n"
		"viWidth: %" PRIu16 "\n"
		"viHeight: %" PRIu16 "\n"
		"xfbMode: %" PRIu32 "\n\n",
		vmode->viTVMode, vmode->fbWidth, vmode->efbHeight,
		vmode->xfbHeight, vmode->viXOrigin, vmode->viYOrigin,
		vmode->viWidth, vmode->viHeight, vmode->xfbMode);
}

static void quit(void)
{
	for (;;)
		VIDEO_WaitVSync();
}


void main(void)
{
	initialize_platform();

	if (!loadrom(tetris)) {
		logerror("Couldn't load rom!\n");
		quit();
	}

	resetcpu();
	resetppu();
	resetapu();

	const uint32_t frameclks = CPU_FREQ / 60;
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
		static int old_buttons[2] = { 0, 0 };
		const int buttons[2] = { PAD_ButtonsHeld(0), PAD_ButtonsHeld(1) };

		if (buttons[0] != old_buttons[0]) {
			if (buttons[0]&PAD_TRIGGER_L) {
				VIDEO_SetNextFramebuffer(console_fb);
				VIDEO_Flush();
			} else if (buttons[0]&PAD_TRIGGER_R) {
				VIDEO_SetNextFramebuffer(gc_fb);
				VIDEO_Flush();
			}
		}

		for (unsigned i = 0; i < JOYPAD_NJOYPADS; ++i) {
			if (buttons[i] == old_buttons[i])
				continue;

			old_buttons[i] = buttons[i];

			gc_padstate[i] =
			  ((buttons[i]&PAD_BUTTON_A) != 0)<<KEY_A          |
			  ((buttons[i]&PAD_BUTTON_B) != 0)<<KEY_B          |
			  ((buttons[i]&PAD_TRIGGER_Z) != 0)<<KEY_SELECT    |
			  ((buttons[i]&PAD_BUTTON_START) != 0)<<KEY_START  |
			  ((buttons[i]&PAD_BUTTON_LEFT) != 0)<<KEY_LEFT    |
			  ((buttons[i]&PAD_BUTTON_RIGHT) != 0)<<KEY_RIGHT  |
			  ((buttons[i]&PAD_BUTTON_UP) != 0)<<KEY_UP        |
			  ((buttons[i]&PAD_BUTTON_DOWN) != 0)<<KEY_DOWN;
		}

		static int fps = 0;
		static time_t timer = 0;

		++fps;
		const time_t now = time(NULL);
		if ((now - timer) >= 1) {
			loginfo("FPS: %d\r", fps);
			fps = 0;
			timer = now;
		}

		VIDEO_WaitVSync();
	}
}
