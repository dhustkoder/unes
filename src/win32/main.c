#include <Windows.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "unes.h"

extern bool init_video_system(HINSTANCE hInstance, int nCmdShow);
extern bool video_win_update(void);
extern void video_start_frame(void);
extern void video_end_frame(void);
extern void term_video_system(void);


extern bool init_audio_system(void);
extern void term_audio_system(void);
extern void internal_audio_sync(void);


extern void init_log_system(void);
extern void term_log_system(void);
extern void internal_logger(LoggerID id, const char* fmt, ...);



static void term_platform(void)
{
	log_info("Terminating Win32 Platform");
	
	term_video_system();
	term_log_system();
}

static bool init_plarform(
	HINSTANCE hInstance,
	const int nCmdShow
)
{
	init_log_system();
	
	log_info("Initializing Win32 Platform");
	
	if (!init_video_system(hInstance, nCmdShow))
		return false;
	if (!init_audio_system())
		return false;

	WSADATA dummy;
	int err;
	if ((err = WSAStartup(MAKEWORD(1, 0), &dummy)) != 0) {
		log_error("Couldn't initialize WSA: %d", err);
		return false;
	}

	return true;
}

static uint8_t* read_file(const char* path)
{
	uint8_t* buffer = NULL;
	
	OFSTRUCT ofstruct;
	HANDLE file = (HANDLE)OpenFile(path, &ofstruct, OF_READ);
	if (!file) {
		log_error("Failed to open ROM file");
		goto Lfail;
	}

	DWORD file_size = GetFileSize(file, NULL);

	if (file_size == INVALID_FILE_SIZE) {
		log_error("Failed to get ROM file size");
		goto Lfail;
	}

	buffer = malloc(sizeof(uint8_t) * file_size + 1);
	if (buffer == NULL) {
		log_error("Failed to allocate memory for ROM data!");
		goto Lfail;
	}

	DWORD bytes_read;
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

#if 1
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
)
{
	((void)hPrevInstance);
	((void)lpCmdLine);


	if (!init_plarform(hInstance, nCmdShow))
		return EXIT_FAILURE;
	
	atexit(term_platform);
	
	uint8_t* rom_buffer = read_file(lpCmdLine);
	if (!rom_buffer)
		return EXIT_FAILURE;

	rom_load(rom_buffer);
	cpu_reset();
	ppu_reset();
	apu_reset();

	const int nes_ticks_per_frame = NES_CPU_FREQ / 60;
	int nes_ticks = 0;
	
	while (video_win_update()) {
		video_start_frame();
		do {
			const short step_ticks = cpu_step();
			ppu_step((step_ticks<<1) + step_ticks);
			apu_step(step_ticks);
			nes_ticks += step_ticks;
		} while (nes_ticks < nes_ticks_per_frame);
		nes_ticks -= nes_ticks_per_frame;
		video_end_frame();
		audio_sync();
	}
	
	rom_unload();
	free(rom_buffer);
	return EXIT_SUCCESS;
}


#else

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
	init_audio_system();
	init_log_system();


	static audio_sample_t sinewave[AUDIO_BUFFER_SAMPLE_COUNT];
	float freq      = 240;
	float amp       = 0.5;
	float phase     = 0.0f;
	float time      = 0.0f;
	float dt        = 1.f / 44100.f;
	float double_pi = M_PI * 2;


	for (;;) {
		for (int i = 0; i < AUDIO_BUFFER_SAMPLE_COUNT; ++i) {
			float value = amp * sinf(double_pi * freq * time + phase);
			sinewave[i] = 8000 * value;
			time += dt;
		}
		internal_audio_push_buffer(sinewave);
		audio_sync();
	}




	term_audio_system();
}

#endif
