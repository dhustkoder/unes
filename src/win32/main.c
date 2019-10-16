#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "internal_log.h"




static void term_platform(void)
{
	log_info("Terminating Win32 Platform");
	
	term_log_system();
}

static BOOL init_plarform(HINSTANCE hInstance)
{
	init_log_system();
	
	log_info("Initializing Win32 Platform");
	
	if (!init_video_system(hInstance))
		return 0;
	
	
	atexit(term_platform);
	
	return 1;
}




int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine,
					 int nCmdShow)
{
	if (!init_plarform(hInstance))
		return EXIT_FAILURE;
	
	log_info("Hello Win32");
	
	while (video_win_update()) {
		video_start_frame();
		video_end_frame();
	}
	
	return EXIT_SUCCESS;
}


