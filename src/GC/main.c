#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>


static void initialize_platform(void)
{
	VIDEO_Init();
	PAD_Init();
	
	GXRModeObj* const rmode = VIDEO_GetPreferredMode(NULL);
	void* const xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	console_init(xfb, 20, 20, 
		    rmode->fbWidth,
		    rmode->xfbHeight,
		    rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if (rmode->viTVMode&VI_NON_INTERLACE)
		VIDEO_WaitVSync();
}


__attribute__((noreturn)) void main(void)
{
	initialize_platform();
	for (;;) {
		VIDEO_WaitVSync();
		PAD_ScanPads();

		const int buttonsDown = PAD_ButtonsDown(0);
		
		if (buttonsDown&PAD_BUTTON_A) {
			// button A pressed
		}
	}
}
