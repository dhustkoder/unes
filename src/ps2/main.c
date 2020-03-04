#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <audsrv.h>
#include <kernel.h>
#include <sifrpc.h>
#include <iopheap.h>
#include <loadfile.h>
#include "rom_data.h"
#include "log.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "rom.h"


#define NES_RGB(r, g, b) ( ((u64)r)|(((u64)g)<<8)|(((u64)b)<<16) )

const u32 nes_rgb_palette[0x40] = { 
	NES_RGB(124,124,124), NES_RGB(0,0,252),     NES_RGB(0,0,188),     NES_RGB(68,40,188),
	NES_RGB(148,0,132),   NES_RGB(168,0,32),    NES_RGB(168,16,0),    NES_RGB(136,20,0),
	NES_RGB(80,48,0),     NES_RGB(0,120,0),     NES_RGB(0,104,0),     NES_RGB(0,88,0),
	NES_RGB(0,64,88),     NES_RGB(0,0,0),       NES_RGB(0,0,0),       NES_RGB(0,0,0),
	NES_RGB(188,188,188), NES_RGB(0,120,248),   NES_RGB(0,88,248),    NES_RGB(104,68,252), 
	NES_RGB(216,0,204),   NES_RGB(228,0,88),    NES_RGB(248,56,0),    NES_RGB(228,92,16),
	NES_RGB(172,124,0),   NES_RGB(0,184,0),     NES_RGB(0,168,0),     NES_RGB(0,168,68),
	NES_RGB(0,136,136),   NES_RGB(0,0,0),       NES_RGB(0,0,0),       NES_RGB(0,0,0),
	NES_RGB(248,248,248), NES_RGB(60,188,252),  NES_RGB(104,136,252), NES_RGB(152,120,248), 
	NES_RGB(248,120,248), NES_RGB(248,88,152),  NES_RGB(248,120,88),  NES_RGB(252,160,68),
	NES_RGB(248,184,0),   NES_RGB(184,248,24),  NES_RGB(88,216,84),   NES_RGB(88,248,152),
	NES_RGB(0,232,216),   NES_RGB(120,120,120), NES_RGB(0,0,0),       NES_RGB(0,0,0),
	NES_RGB(252,252,252), NES_RGB(164,228,252), NES_RGB(184,184,248), NES_RGB(216,184,248),
	NES_RGB(248,184,248), NES_RGB(248,164,192), NES_RGB(240,208,176), NES_RGB(252,224,168), 
	NES_RGB(248,216,120), NES_RGB(216,248,120), NES_RGB(184,248,184), NES_RGB(184,248,216),
	NES_RGB(0,252,252),   NES_RGB(248,216,248), NES_RGB(0,0,0),       NES_RGB(0,0,0)
};

GSGLOBAL* gsGlobal;
GSTEXTURE fb;

static void platform_init(void)
{

	dmaKit_init(
		D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF
	);

	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsGlobal = gsKit_init_global();

	//  By default the gsKit_init_global() uses an autodetected interlaced field mode
	//  To set a new mode set these five variables for the resolution desired and
	//  mode desired

	//  Some examples
	//  Make sure that gsGlobal->Height is a factor of the mode's gsGlobal->DH

	gsGlobal->Mode = GS_MODE_NTSC;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FIELD;
	gsGlobal->DrawField = GS_FIELD_EVEN;
	gsGlobal->Width = NES_SCR_WIDTH;
	gsGlobal->Height = NES_SCR_HEIGHT;

	// gsGlobal->Mode = GS_MODE_PAL;
	// gsGlobal->Interlace = GS_INTERLACED;
	// gsGlobal->Field = GS_FIELD;
	// gsGlobal->Width = 640;
	// gsGlobal->Height = 512;

	// gsGlobal->Mode = GS_MODE_DTV_480P;
	// gsGlobal->Interlace = GS_NONINTERLACED;
	// gsGlobal->Field = GS_FRAME;
	// gsGlobal->Width = 720;
	// gsGlobal->Height = 480;

	// gsGlobal->Mode = GS_MODE_DTV_1080I;
	// gsGlobal->Interlace = GS_INTERLACED;
	// gsGlobal->Field = GS_FIELD;
	// gsGlobal->Width = 640;
	// gsGlobal->Height = 540;
	// gsGlobal->PSM = GS_PSM_CT16;
	// gsGlobal->PSMZ = GS_PSMZ_16;
	// gsGlobal->Dithering = GS_SETTING_ON;

	//  A width of 640 would work as well
	//  However a height of 720 doesn't seem to work well
	// gsGlobal->Mode = GS_MODE_DTV_720P;
	// gsGlobal->Interlace = GS_NONINTERLACED;
	// gsGlobal->Field = GS_FRAME;
	// gsGlobal->Width = 640;
	// gsGlobal->Height = 360;
	// gsGlobal->PSM = GS_PSM_CT16;
	// gsGlobal->PSMZ = GS_PSMZ_16;

	//  You can use these to turn off Z/Double Buffering. They are on by default.
	//  gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	//  gsGlobal->ZBuffering = GS_SETTING_OFF;

	//  This makes things look marginally better in half-buffer mode...
	//  however on some CRT and all LCD, it makes a really horrible screen shake.
	//  Uncomment this to disable it. (It is on by default)
	//  gsGlobal->DoSubOffset = GS_SETTING_OFF;


	/* initialize screen */
	gsGlobal->PSM = GS_PSM_CT32;
	gsGlobal->ZBuffering = GS_SETTING_OFF; /* spare some vram */
	//  If we disable double buffering, we can't fill the frame fast enough.
	//  When this happens, we get a line through the top texture about 20% up
	//  from the bottom of the screen.
	// gsGlobal->DoubleBuffering = GS_SETTING_OFF; /* only one screen */

	gsKit_init_screen(gsGlobal);

	gsKit_mode_switch(gsGlobal, GS_PERSISTENT);

	fb.Width = NES_SCR_WIDTH;
	fb.Height = NES_SCR_HEIGHT;
	fb.PSM = GS_PSM_CT32;
	fb.ClutPSM = 0;

	fb.Mem = memalign(128, gsKit_texture_size_ee(fb.Width, fb.Height, fb.PSM));
	fb.Clut = NULL;//memalign(128, gsKit_texture_size_ee(0x40, 1, fb.ClutPSM));

	fb.VramClut = NULL;// gsKit_vram_alloc(gsGlobal, gsKit_texture_size(16, 16, fb.ClutPSM), GSKIT_ALLOC_USERBUFFER);
	fb.Vram = gsKit_vram_alloc(gsGlobal, gsKit_texture_size(fb.Width, fb.Height, fb.PSM), GSKIT_ALLOC_USERBUFFER);

	fb.Filter = GS_FILTER_LINEAR; /* enable bilinear filtering */

	gsKit_setup_tbw(&fb);

	log_info("Hello Sony Playstation 2\n");

	log_info(
		"Display Mode: %d\n"
		"Interlaced: %d\n"
		"DW: %d\n"
		"DH: %d\n"
		"Width: %d\n"
		"Height: %d\n",
		gsGlobal->Mode,
		gsGlobal->Interlace,
		gsGlobal->DW,
		gsGlobal->DH,
		gsGlobal->Width,
		gsGlobal->Height
	);

	log_info("CLUT Texture:\n");
	log_info("\tHost  start: 0x%08x, end: 0x%08x\n", (unsigned)fb.Mem, (unsigned)(gsKit_texture_size_ee(fb.Width, fb.Height, fb.PSM) + fb.Mem));
	log_info("\tLocal start: 0x%08x, end: 0x%08x\n", (unsigned)fb.Vram, (unsigned)(gsKit_texture_size(fb.Width, fb.Height, fb.PSM) + fb.Vram));
	log_info("\tWidth - %d : Height - %d : TBW - %d : Page - %d : Block %d\n", fb.Width, fb.Height, fb.TBW, (fb.Vram / 8192), (fb.Vram / 256));
	log_info("CLUT Pallete:\n");
	log_info("\tHost  start: 0x%08x, end: 0x%08x\n", (unsigned)fb.Clut, (unsigned)(gsKit_texture_size_ee(16, 16, GS_PSM_CT32) + fb.Clut));
	log_info("\tLocal start: 0x%08x, end: 0x%08x\n", (unsigned)fb.VramClut, (unsigned)(gsKit_texture_size(16, 16, GS_PSM_CT32) + fb.VramClut));
	log_info("\tWidth - %d : Height - %d : TBW - %d : Page - %d : Block %d\n", 16, 16, 1, (fb.VramClut / 8192), (fb.VramClut / 256));
	log_info("VRAM Alignment Check - Value of \"0\" is OKAY! Anything else is BAD!\n");
	log_info("VRAM - CLUT Pallete - Start Address Aligned: %d\n", fb.VramClut % GS_VRAM_BLOCKSIZE_256);
	log_info("VRAM - CLUT Texture - Start Address Aligned: %d\n", fb.Vram % GS_VRAM_BLOCKSIZE_256);


	/* clear buffer */
	gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00));

	/* render frame buffer */
	gsKit_prim_sprite_texture( gsGlobal,	&fb,
		0.0f, /* X1 */
		0.0f, /* Y1 */
		0.0f, /* U1 */
		0.0f, /* V1 */
		gsGlobal->Width, /* X2 */
		gsGlobal->Height, /* Y2 */
		fb.Width, /* U2 */
		fb.Height, /* V2*/
		0, /* Z */
		GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x80) /* RGBAQ */
	);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

}


int main(void)
{

	platform_init();

	rom_load(rom_data);
	cpu_reset();
	apu_reset();
	ppu_reset();

	const s32 ticks_per_sec = NES_CPU_FREQ / 60;
	s32 ticks = 0;

	for (;;) {

		do {
			const short step_ticks = cpu_step();
			ppu_step((step_ticks<<1) + step_ticks);
			apu_step(step_ticks);
			ticks += step_ticks;
		} while (ticks < ticks_per_sec);

		ticks -= ticks_per_sec;


		/* generate next frame */
		/* upload new frame buffer */
		gsKit_texture_upload(gsGlobal, &fb);

		/* execute render queue */
		gsKit_queue_exec(gsGlobal);

		/* vsync and flip buffer */
		gsKit_sync_flip(gsGlobal);
	}




	return 0;
}

