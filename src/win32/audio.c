#include <Windows.h>
#include "platform.h"

#define QUEUE_FRAME_COUNT (4)

typedef struct {
	WAVEHDR header;
	audio_sample_t data[AUDIO_BUFFER_SAMPLE_COUNT];
} AudioFrame;

typedef struct {
	AudioFrame frames[QUEUE_FRAME_COUNT];
	int writting_idx;
	int queued_samples;
} AudioQueue;

static HWAVEOUT wout;
static AudioQueue audio_queue;


static void audio_frame_setup_and_send(AudioFrame* frame, const void* data)
{
	waveOutUnprepareHeader(wout, &frame->header, sizeof(frame->header)); 

	ZeroMemory(&frame->header, sizeof(frame->header));
	
	frame->header.dwBufferLength = AUDIO_BUFFER_SIZE;
	frame->header.lpData = frame->data;
	
	memcpy(frame->data, data, AUDIO_BUFFER_SIZE);

	waveOutPrepareHeader(wout, &frame->header, sizeof(frame->header)); 
	waveOutWrite(wout, &frame->header, sizeof(frame->header));
}

static void audio_queue_reset(void)
{
	ZeroMemory(&audio_queue, sizeof(audio_queue));
	audio_queue.queued_samples = 0;
	audio_queue.writting_idx = 0;
}

static void audio_enqueue(const void* framebuffer)
{
	AudioFrame* frame = &audio_queue.frames[audio_queue.writting_idx];
	audio_frame_setup_and_send(frame, framebuffer);

	audio_queue.queued_samples += AUDIO_BUFFER_SAMPLE_COUNT;

	++audio_queue.writting_idx;
	if (audio_queue.writting_idx >= QUEUE_FRAME_COUNT)
		audio_queue.writting_idx = 0;
}

static DWORD get_enqueued_samples_count(void)
{
	static DWORD last_sample_time = 0;
	static MMTIME mmt = {.wType = TIME_SAMPLES};

	waveOutGetPosition(wout, &mmt, sizeof(mmt));

	audio_queue.queued_samples -= (mmt.u.sample - last_sample_time);

	last_sample_time = mmt.u.sample;

	return audio_queue.queued_samples;
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
	audio_enqueue(data);
}

void audio_sync(void)
{
	const DWORD max_queued_samples = AUDIO_BUFFER_SAMPLE_COUNT * QUEUE_FRAME_COUNT;
	const double sample_time = 1.0 / AUDIO_SAMPLES_PER_SEC;
	for (;;) {
		const DWORD queued_samples = get_enqueued_samples_count();		
		if (queued_samples < max_queued_samples) {
			break;
		} else {
			Sleep(sample_time * (queued_samples - max_queued_samples) * 1000);
		}
	}
}

