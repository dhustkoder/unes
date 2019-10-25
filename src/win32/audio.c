#include <Windows.h>
#include <assert.h>
#include "log.h"
#include "internal_audio.h"


#define QUEUE_SIZE (32)

static HWAVEOUT h_waveout;
static WAVEFORMATEX wfx;
static WAVEHDR header[QUEUE_SIZE];
static int writing_idx = 0;
static volatile int reading_idx = 0;
static audio_t data[QUEUE_SIZE][AUDIO_BUFFER_SIZE];
static CRITICAL_SECTION cso;

static void send_data_to_device(const int idx)
{
	MMRESULT err;
	((void)err);

	ZeroMemory(&header[idx], sizeof(WAVEHDR));
	header[idx].dwBufferLength = sizeof(audio_t) * AUDIO_BUFFER_SIZE;
	header[idx].lpData = (LPSTR) data[idx];

	err = waveOutPrepareHeader(h_waveout, &header[idx], sizeof(WAVEHDR)); 
	assert(err == MMSYSERR_NOERROR);

	err = waveOutWrite(h_waveout, &header[idx], sizeof(WAVEHDR));
	assert(err == MMSYSERR_NOERROR);
}


BOOL init_audio_system(void)
{
	InitializeCriticalSection(&cso);
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
			0, 0, 0) != MMSYSERR_NOERROR) { 
		log_error("unable to open WAVE_MAPPER device");
		return 0;
	}

	return 1;
}

void term_audio_system(void)
{
	waveOutClose(h_waveout);
}

void internal_audio_push_buffer(const audio_t* const unes_data)
{
	memcpy(data[writing_idx], unes_data, sizeof(audio_t) * AUDIO_BUFFER_SIZE);
	writing_idx = (writing_idx + 1) % QUEUE_SIZE;
}

void internal_audio_sync(void)
{
	if (writing_idx == reading_idx) 
		return;
	send_data_to_device(reading_idx);
	reading_idx = (reading_idx + 1) % QUEUE_SIZE;
}



