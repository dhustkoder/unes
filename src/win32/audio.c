#include <Windows.h>
#include <assert.h>
#include "log.h"
#include "internal_audio.h"



static HWAVEOUT h_waveout;
static WAVEFORMATEX wfx;
static WAVEHDR header;
static HANDLE hsem;
static audio_t data[AUDIO_BUFFER_SIZE];


static void send_data_to_device(void)
{
	MMRESULT err;
	((void)err);

	err = waveOutWrite(h_waveout, &header, sizeof(WAVEHDR));
	assert(err == MMSYSERR_NOERROR);
}

static void init_device_cycle(void)
{
	MMRESULT err;
	((void)err);

	ZeroMemory(&header, sizeof(WAVEHDR));
	header.dwBufferLength = sizeof(audio_t) * AUDIO_BUFFER_SIZE;
	header.lpData = (LPSTR) data;

	err = waveOutPrepareHeader(h_waveout, &header, sizeof(WAVEHDR)); 
	assert(err == MMSYSERR_NOERROR);

	send_data_to_device();
}

static void CALLBACK wave_out_proc(HWAVEOUT  hwo,
                                   UINT      uMsg,
                                   DWORD_PTR dwInstance,
                                   DWORD_PTR dwParam1,
                                   DWORD_PTR dwParam2)
{
	((void)hwo);
	((void)dwInstance);
	((void)dwParam1);
	((void)dwParam2);

	if (uMsg != WOM_DONE)
		return;

	ReleaseSemaphore(hsem, 1, NULL);
}


BOOL init_audio_system(void)
{
	hsem = CreateSemaphore(NULL, 0, 1, NULL);
	if (!hsem) {
		log_error("Failed to create semaphore: %d", GetLastError());
		return 0;
	}

	wfx.nSamplesPerSec = AUDIO_FREQUENCY; 
	wfx.wBitsPerSample = sizeof(audio_t) * 8; 
	wfx.nChannels = 1; 

	wfx.cbSize = 0; 
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	
	if (waveOutOpen(&h_waveout,
			WAVE_MAPPER,
	                &wfx,
			(DWORD_PTR)wave_out_proc, 0,
	                CALLBACK_FUNCTION|
			WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR) { 
		log_error("unable to open WAVE_MAPPER device");
		return 0;
	}

	init_device_cycle();

	return 1;
}

void term_audio_system(void)
{
	waveOutClose(h_waveout);
	CloseHandle(hsem);
}

void internal_audio_play_pcm(const audio_t* const unes_data)
{
	WaitForSingleObject(hsem, INFINITE);
	memcpy(data, unes_data, sizeof(audio_t) * AUDIO_BUFFER_SIZE);
	send_data_to_device();
}


