#include <Windows.h>
#include <assert.h>
#include "log.h"
#include "internal_audio.h"



static HWAVEOUT h_waveout; /* device handle */
static WAVEFORMATEX wfx;   /* look this up in your documentation */
static WAVEHDR header;
static volatile BOOL isplaying = 0;
static audio_t data[AUDIO_BUFFER_SIZE];



static void send_data_to_device(void)
{
	MMRESULT err;

	while (isplaying)
		;

	header.dwBufferLength = sizeof(audio_t) * AUDIO_BUFFER_SIZE;
	header.lpData = (LPSTR) data;

	err = waveOutPrepareHeader(h_waveout, &header, sizeof(WAVEHDR)); 
	if (err != MMSYSERR_NOERROR)
		log_error("waveOutPrepareHeader Error: %d", err);

	err = waveOutWrite(h_waveout, &header, sizeof(WAVEHDR));
	if (err != MMSYSERR_NOERROR)
		log_error("waveOutWrite Error: %d", err);

	isplaying = 1;
}

static void init_device_cycle(void)
{
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
	if (uMsg == WOM_DONE)
		isplaying = 0;
}


BOOL init_audio_system(void)
{
	ZeroMemory(&header, sizeof(WAVEHDR));

	wfx.nSamplesPerSec = AUDIO_FREQUENCY; 
	wfx.wBitsPerSample = sizeof(audio_t) * 8; 
	wfx.nChannels = 1; 

	wfx.cbSize = 0; 
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	
	if (waveOutOpen(&h_waveout, WAVE_MAPPER,
	                &wfx, (DWORD_PTR)wave_out_proc, 0,
	                CALLBACK_FUNCTION) != MMSYSERR_NOERROR) { 
		log_error("unable to open WAVE_MAPPER device");
		return 0;
	}

	init_device_cycle();

	return 1;
}

void term_audio_system(void)
{
	waveOutClose(h_waveout);
}

void internal_audio_play_pcm(const audio_t* const unes_data)
{
	memcpy(data, unes_data, sizeof(audio_t) * AUDIO_BUFFER_SIZE);
	send_data_to_device();
}


