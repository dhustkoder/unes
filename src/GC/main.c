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


#define RGB_TO_Y1CBY2CR(r, g, b)                                                    \
  (((299 * r + 587 * g + 114 * b) / 1000)<<24) |                                    \
  (((((-16874 * r - 33126 * g + 50000 * b + 12800000) / 100000)<<1)>>1)<<16) |      \
  (((299 * r + 587 * g + 114 * b) / 1000) << 8) |                                   \
  ((((50000 * r - 41869 * g - 8131 * b + 12800000) / 100000)<<1)>>1)

#define RGB(r, g, b) RGB_TO_Y1CBY2CR(r, g, b)

const uint32_t gc_nes_colors[0x40] = { 
	RGB(124,124,124), RGB(0,0,252), RGB(0,0,188), RGB(68,40,188), RGB(148,0,132), 
	RGB(168,0,32), RGB(168,16,0), RGB(136,20,0), RGB(80,48,0), RGB(0,120,0), 
	RGB(0,104,0), RGB(0,88,0), RGB(0,64,88), RGB(0,0,0), RGB(0,0,0), 
	RGB(0,0,0), RGB(188,188,188), RGB(0,120,248), RGB(0,88,248), RGB(104,68,252), 
	RGB(216,0,204), RGB(228,0,88), RGB(248,56,0), RGB(228,92,16), RGB(172,124,0), 
	RGB(0,184,0), RGB(0,168,0), RGB(0,168,68), RGB(0,136,136), RGB(0,0,0), RGB(0,0,0), 
	RGB(0,0,0), RGB(248,248,248), RGB(60,188,252), RGB(104,136,252), RGB(152,120,248), 
	RGB(248,120,248), RGB(248,88,152), RGB(248,120,88), RGB(252,160,68), RGB(248,184,0), 
	RGB(184,248,24), RGB(88,216,84), RGB(88,248,152), RGB(0,232,216), RGB(120,120,120), 
	RGB(0,0,0), RGB(0,0,0), RGB(252,252,252), RGB(164,228,252), RGB(184,184,248), 
	RGB(216,184,248), RGB(248,184,248), RGB(248,164,192), RGB(240,208,176), RGB(252,224,168), 
	RGB(248,216,120), RGB(216,248,120), RGB(184,248,184), RGB(184,248,216), RGB(0,252,252), 
	RGB(248,216,248), RGB(0,0,0), RGB(0,0,0)
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
