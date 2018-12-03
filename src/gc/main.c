#include <ogcsys.h>
#include <gccore.h>
#include <inttypes.h>
#include <stdnoreturn.h>
#include <string.h>
#include "log.h"
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "megamanii.h"

#define RGB_TO_Y1CBY2CR(r, g, b)                                                    \
  ((((299 * r + 587 * g + 114 * b) / 1000)<<24) |                                   \
  (((((-16874 * r - 33126 * g + 50000 * b + 12800000) / 100000)<<1)>>1)<<16) |      \
  (((299 * r + 587 * g + 114 * b) / 1000)<<8) |                                     \
  ((((50000 * r - 41869 * g - 8131 * b + 12800000) / 100000)<<1)>>1))

#define RGB(r, g, b) RGB_TO_Y1CBY2CR(r, g, b)

const uint32_t gc_nes_colors[0x40] = { 
	RGB(124,124,124), RGB(0,0,252),     RGB(0,0,188),     RGB(68,40,188),
	RGB(148,0,132),   RGB(168,0,32),    RGB(168,16,0),    RGB(136,20,0),
	RGB(80,48,0),     RGB(0,120,0),     RGB(0,104,0),     RGB(0,88,0),
	RGB(0,64,88),     RGB(0,0,0),       RGB(0,0,0),       RGB(0,0,0),
	RGB(188,188,188), RGB(0,120,248),   RGB(0,88,248),    RGB(104,68,252), 
	RGB(216,0,204),   RGB(228,0,88),    RGB(248,56,0),    RGB(228,92,16),
	RGB(172,124,0),   RGB(0,184,0),     RGB(0,168,0),     RGB(0,168,68),
	RGB(0,136,136),   RGB(0,0,0),       RGB(0,0,0),       RGB(0,0,0),
	RGB(248,248,248), RGB(60,188,252),  RGB(104,136,252), RGB(152,120,248), 
	RGB(248,120,248), RGB(248,88,152),  RGB(248,120,88),  RGB(252,160,68),
	RGB(248,184,0),   RGB(184,248,24),  RGB(88,216,84),   RGB(88,248,152),
	RGB(0,232,216),   RGB(120,120,120), RGB(0,0,0),       RGB(0,0,0),
	RGB(252,252,252), RGB(164,228,252), RGB(184,184,248), RGB(216,184,248),
	RGB(248,184,248), RGB(248,164,192), RGB(240,208,176), RGB(252,224,168), 
	RGB(248,216,120), RGB(216,248,120), RGB(184,248,184), RGB(184,248,216),
	RGB(0,252,252),   RGB(248,216,248), RGB(0,0,0),       RGB(0,0,0)
};

uint8_t gc_nes_padstate[2];
uint32_t* gc_fb;
GXRModeObj* gc_vmode;
static void* console_fb;


static void initialize_platform(void)
{
	VIDEO_Init();
	PAD_Init();
	
	gc_vmode= VIDEO_GetPreferredMode(NULL);
	VIDEO_Configure(gc_vmode);

	gc_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gc_vmode));
	console_fb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gc_vmode));

	console_init(console_fb, 0, 0,
	             gc_vmode->fbWidth,
	             gc_vmode->xfbHeight,
	             gc_vmode->fbWidth * 2);

	VIDEO_ClearFrameBuffer(gc_vmode, gc_fb, COLOR_BLACK);
	VIDEO_SetNextFramebuffer(gc_fb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	log_info("GC VMODE INFO:\n"
		"viTVMode: %" PRIu32 "\n"
	        "fbWidth: %" PRIu16 "\n"
		"efbHeight: %" PRIu16 "\n"
		"xfbHeight: %" PRIu16 "\n"
		"viXOrigin: %" PRIu16 "\n"
		"viYOrigin: %" PRIu16 "\n"
		"viWidth: %" PRIu16 "\n"
		"viHeight: %" PRIu16 "\n"
		"xfbMode: %" PRIu32 "\n\n",
		gc_vmode->viTVMode, gc_vmode->fbWidth, gc_vmode->efbHeight,
		gc_vmode->xfbHeight, gc_vmode->viXOrigin, gc_vmode->viYOrigin,
		gc_vmode->viWidth, gc_vmode->viHeight, gc_vmode->xfbMode);
}

static void quit(void)
{
	for (;;)
		VIDEO_WaitVSync();
}

static void update_pad_events(void)
{
	PAD_ScanPads();
	const int buttons[2] = { PAD_ButtonsHeld(0), PAD_ButtonsHeld(1) };

	if (buttons[0]&PAD_TRIGGER_L) {
		VIDEO_SetNextFramebuffer(console_fb);
		VIDEO_Flush();
	} else if (buttons[0]&PAD_TRIGGER_R) {
		VIDEO_SetNextFramebuffer(gc_fb);
		VIDEO_Flush();
	}

	for (unsigned i = 0; i < 2; ++i) {
		gc_nes_padstate[i] =
			((buttons[i]&PAD_BUTTON_A) != 0)<<KEY_A          |
			((buttons[i]&PAD_BUTTON_B) != 0)<<KEY_B          |
			((buttons[i]&PAD_TRIGGER_Z) != 0)<<KEY_SELECT    |
			((buttons[i]&PAD_BUTTON_START) != 0)<<KEY_START  |
			((buttons[i]&PAD_BUTTON_LEFT) != 0)<<KEY_LEFT    |
			((buttons[i]&PAD_BUTTON_RIGHT) != 0)<<KEY_RIGHT  |
			((buttons[i]&PAD_BUTTON_UP) != 0)<<KEY_UP        |
			((buttons[i]&PAD_BUTTON_DOWN) != 0)<<KEY_DOWN;
	}
}


noreturn void main(void)
{
	initialize_platform();

	if (!rom_load(megamanii)) {
		log_error("Couldn't load rom!\n");
		quit();
	}

	cpu_reset();
	ppu_reset();
	apu_reset();

	const int nes_clock_div = (gc_vmode->viTVMode&VI_NON_INTERLACE) ? 60 : 50;
	const int ticks_per_sec = NES_CPU_FREQ / nes_clock_div;

	int clk = 0;
	for (;;) {
		do {
			const short ticks = cpu_step();
			ppu_step((ticks<<1) + ticks);
			apu_step(ticks);
			clk += ticks;
		} while (clk < ticks_per_sec);
		clk -= ticks_per_sec;

		update_pad_events();
		VIDEO_WaitVSync();
	}

	rom_unload();
	quit();
}
