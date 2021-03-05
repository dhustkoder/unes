#include <Windows.h>
#include "platform.h"


static HWAVEOUT h_waveout;
static WAVEHDR header;
static HANDLE buffer_done_ev; 


void CALLBACK waveOutProc(
   HWAVEOUT  hwo,
   UINT      uMsg,
   DWORD_PTR dwInstance,
   DWORD_PTR dwParam1,
   DWORD_PTR dwParam2
)
{
	if (uMsg == WOM_DONE) {
		SetEvent(buffer_done_ev);
	}
}

BOOL init_audio_system(void)
{
    buffer_done_ev = CreateEvent(NULL, FALSE, FALSE, TEXT("BufferDoneEv")); 

	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));

	wfx.wFormatTag = WAVE_FORMAT_PCM;

	wfx.nSamplesPerSec = AUDIO_FREQUENCY;
	wfx.wBitsPerSample = 16; 
	wfx.nBlockAlign = 2;
	wfx.nChannels = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	
	if (waveOutOpen(&h_waveout, WAVE_MAPPER, &wfx, waveOutProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
		log_error("unable to open WAVE_MAPPER device");
		return 0;
	}

	return 1;
}

void term_audio_system(void)
{
	waveOutClose(h_waveout);
}



void internal_audio_push_buffer(const audio_t* data)
{
	MMRESULT err;
	((void)err);

	ZeroMemory(&header, sizeof(WAVEHDR));
	header.dwBufferLength = sizeof(audio_t) * AUDIO_BUFFER_SIZE;
	header.lpData = data;
	header.dwFlags = WHDR_INQUEUE;

	err = waveOutPrepareHeader(h_waveout, &header, sizeof(header)); 
	assert(err == MMSYSERR_NOERROR);

	err = waveOutWrite(h_waveout, &header, sizeof(header));
	assert(err == MMSYSERR_NOERROR);

	WaitForSingleObject(buffer_done_ev, INFINITE);
}

