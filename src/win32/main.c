#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "internal_log.h"
#include "internal_video.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "rom.h"

static void term_platform(void)
{
	log_info("Terminating Win32 Platform");
	
	term_video_system();
	term_log_system();
}

static BOOL init_plarform(HINSTANCE hInstance,
                          const int nCmdShow)
{
	init_log_system();
	
	log_info("Initializing Win32 Platform");
	
	if (!init_video_system(hInstance, nCmdShow))
		return 0;
	
	
	atexit(term_platform);
	
	return 1;
}




int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
					 LPSTR lpCmdLine,
					 const int nCmdShow)
{
	if (!init_plarform(hInstance, nCmdShow))
		return EXIT_FAILURE;
	
	log_info("Hello Win32");


	OFSTRUCT ofstruct;
	HANDLE file = OpenFile("dk.nes", &ofstruct, OF_READ);

	if (!file) {
		log_error("Failed to open ROM file");
		return EXIT_FAILURE;
	}

	const DWORD file_size = GetFileSize(file, NULL);

	if (file_size == INVALID_FILE_SIZE) {
		log_error("Failed to get ROM file size");
		return EXIT_FAILURE;
	}

	const uint8_t* const rom_buffer = malloc(sizeof(uint8_t) * file_size + 1);
	if (rom_buffer == NULL) {
		log_error("Failed to allocate memory for ROM data!");
		return EXIT_FAILURE;
	}

	DWORD bytes_read;

	if (!ReadFile(file, rom_buffer, file_size, &bytes_read, NULL)) {
		log_error("Failed to read ROM file %d", GetLastError());
		return EXIT_FAILURE;
	}

	CloseHandle(file);

	rom_load(rom_buffer);
	cpu_reset();
	ppu_reset();
	apu_reset();

	const int ticks_per_sec = NES_CPU_FREQ / 60;
	int ticks = 0;

	
	while (video_win_update()) {
		video_start_frame();

		do {
			const short step_ticks = cpu_step();
			ppu_step((step_ticks<<1) + step_ticks);
			apu_step(step_ticks);
			ticks += step_ticks;
		} while (ticks < ticks_per_sec);
		
		ticks -= ticks_per_sec;

		video_end_frame();
	}
	
	return EXIT_SUCCESS;
}


