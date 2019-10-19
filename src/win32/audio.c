#include <Windows.h>
#include "log.h"
#include "internal_audio.h"


#define AQUEUE_ENTRIES (128)

static HWAVEOUT h_waveout; /* device handle */
static WAVEFORMATEX wfx;   /* look this up in your documentation */
static MMRESULT result;    /* for waveout return values */
static WAVEHDR header;
static int aqueue_playing_idx = 0;
static int aqueue_idx = 0;
static audio_t aqueue[AUDIO_BUFFER_SIZE * AQUEUE_ENTRIES];


static void writeAudioBlock(const LPSTR block, const DWORD size)
{
	//internal_audio_sync();
	/*
	 * initialise the block header with the size
	 * and pointer.
	 */
	header.dwBufferLength = size;
	header.lpData = block;
	header.dwLoops = 2;
	/*
	 * prepare the block for playback
	 */
	waveOutPrepareHeader(h_waveout, &header, sizeof(WAVEHDR));
	/*
	 * write the block to the device. waveOutWrite returns immediately
	 * unless a synchronous driver is used (not often).
	 */
	waveOutWrite(h_waveout, &header, sizeof(WAVEHDR));
}


BOOL init_audio_system(void)
{

	ZeroMemory(&header, sizeof(WAVEHDR));

	/*
	 * first we need to set up the WAVEFORMATEX structure. 
	 * the structure describes the format of the audio.
	 */
	wfx.nSamplesPerSec = AUDIO_FREQUENCY; /* sample rate */
	wfx.wBitsPerSample = sizeof(audio_t) << 3; /* sample size */
	wfx.nChannels = 1; /* channels*/
	/*
	 * WAVEFORMATEX also has other fields which need filling.
	 * as long as the three fields above are filled this should
	 * work for any PCM (pulse code modulation) format.
	 */
	wfx.cbSize = 0; /* size of _extra_ info */
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	/*
	 * try to open the default wave device. WAVE_MAPPER is
	 * a constant defined in mmsystem.h, it always points to the
	 * default wave device on the system (some people have 2 or
	 * more sound cards).
	 */
	if (waveOutOpen(&h_waveout, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) { 
		log_error("unable to open WAVE_MAPPER device");
		return 0;
	}
	/*
	 * device is now open so print the success message
	 * and then close the device again.
	 */
	return 1;
}

void term_audio_system(void)
{
	waveOutClose(h_waveout);
}

void internal_audio_play_pcm(const audio_t* const unes_data)
{
	const LPSTR data = (LPSTR) (aqueue + (aqueue_idx * AUDIO_BUFFER_SIZE)); 
	memcpy(data, unes_data, AUDIO_BUFFER_SIZE * sizeof(audio_t));
	aqueue_idx = (aqueue_idx + 1) % AQUEUE_ENTRIES;
	internal_audio_sync();
}

void internal_audio_sync(void)
{
	if (aqueue_playing_idx == aqueue_idx ||
	    waveOutUnprepareHeader(h_waveout, &header, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) 
		return;

	const LPSTR data = (LPSTR) (aqueue + (aqueue_playing_idx * AUDIO_BUFFER_SIZE));
	aqueue_playing_idx = (aqueue_playing_idx + 1) % AQUEUE_ENTRIES;
	writeAudioBlock(data, sizeof(audio_t) * AUDIO_BUFFER_SIZE);
}




