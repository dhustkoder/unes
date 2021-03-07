#include <Windows.h>
#include "platform.h"

#define QUEUE_SIZE (3)
static HWAVEOUT wout;
static WAVEHDR header[QUEUE_SIZE];
static audio_sample_t sample_buf[QUEUE_SIZE][AUDIO_BUFFER_SAMPLE_COUNT];
static int writting_buf_idx = 0;
static int queued_samples = 0;

static void advance_writting_idx(void)
{
	++writting_buf_idx;
	if (writting_buf_idx >= QUEUE_SIZE)
		writting_buf_idx = 0;
}

bool init_audio_system(void)
{
	WAVEFORMATEX wfx;
	ZeroMemory(&wfx, sizeof(WAVEFORMATEX));

	wfx.wFormatTag = WAVE_FORMAT_PCM;

	wfx.nSamplesPerSec = AUDIO_SAMPLES_PER_SEC;
	wfx.wBitsPerSample = AUDIO_SAMPLE_SIZE * 8;
	wfx.nBlockAlign = AUDIO_SAMPLE_SIZE * AUDIO_CHANNEL_COUNT;
	wfx.nChannels = AUDIO_CHANNEL_COUNT;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	
	if (waveOutOpen(&wout, WAVE_MAPPER, &wfx, 0, 0, WAVE_FORMAT_DIRECT) != MMSYSERR_NOERROR) {
		log_error("unable to open WAVE_MAPPER device");
		return false;
	}

	return true;
}

void term_audio_system(void)
{
	waveOutClose(wout);
}




void internal_audio_push_buffer(const audio_sample_t* data)
{
	MMRESULT err;

	ZeroMemory(&header[writting_buf_idx], sizeof(WAVEHDR));
	header[writting_buf_idx].dwBufferLength = AUDIO_BUFFER_SIZE;
	header[writting_buf_idx].lpData = &sample_buf[writting_buf_idx][0];
	memcpy(header[writting_buf_idx].lpData, data, AUDIO_BUFFER_SIZE);

	err = waveOutPrepareHeader(wout, &header[writting_buf_idx], sizeof(header[writting_buf_idx])); 
	assert(err == MMSYSERR_NOERROR);

	err = waveOutWrite(wout, &header[writting_buf_idx], sizeof(header[writting_buf_idx]));
	assert(err == MMSYSERR_NOERROR);

	advance_writting_idx();

	queued_samples += AUDIO_BUFFER_SAMPLE_COUNT;
}

void audio_sync(void)
{
	static DWORD last_sample_time = 0;
	const int max_queued_samples = AUDIO_BUFFER_SAMPLE_COUNT * QUEUE_SIZE;
	for (;;) {
		MMTIME mmt = {.wType = TIME_SAMPLES};
		waveOutGetPosition(wout, &mmt, sizeof(mmt));
		queued_samples -= (mmt.u.sample - last_sample_time);
		last_sample_time = mmt.u.sample;		
		if (queued_samples < max_queued_samples)
			break;
	}
}

