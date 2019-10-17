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
		
	return 1;
}

uint8_t* read_file(const char* path)
{
	uint8_t* buffer = NULL;
	HANDLE file = 0;
	OFSTRUCT ofstruct;	
	DWORD file_size, bytes_read;

	file = OpenFile("dk.nes", &ofstruct, OF_READ);
	if (!file) {
		log_error("Failed to open ROM file");
		goto Lfail;
	}

	file_size = GetFileSize(file, NULL);

	if (file_size == INVALID_FILE_SIZE) {
		log_error("Failed to get ROM file size");
		goto Lfail;
	}

	buffer = malloc(sizeof(uint8_t) * file_size + 1);
	if (buffer == NULL) {
		log_error("Failed to allocate memory for ROM data!");
		goto Lfail;
	}


	if (!ReadFile(file, buffer, file_size, &bytes_read, NULL)) {
		log_error("Failed to read ROM file %d", GetLastError());
		goto Lfail;
	}

	CloseHandle(file);
	return buffer;

Lfail:
	log_error("Error Code: %d", GetLastError());
	if (file)
		CloseHandle(file);
	if (buffer)
		free(buffer);
	return NULL;
}


int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     const int nCmdShow)
{
	if (!init_plarform(hInstance, nCmdShow))
		return EXIT_FAILURE;
	
	atexit(term_platform);

	uint8_t* rom_buffer = read_file("dk.nes");

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
	
	rom_unload();
	free(rom_buffer);
	return EXIT_SUCCESS;
}

