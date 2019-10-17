#include <Windows.h>
#include "log.h"
#include "internal_audio.h"




static HWAVEOUT hWaveOut; /* device handle */
static WAVEFORMATEX wfx; /* look this up in your documentation */
static MMRESULT result;/* for waveOut return values */
static WAVEHDR header;

void writeAudioBlock(const LPSTR block, const DWORD size)
{
	/*
	 * wait a while for the block to play then start trying
	 * to unprepare the header. this will fail until the block has
	 * played.
	 */
	while (waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING) {
		// ...
	}

	/*
	 * initialise the block header with the size
	 * and pointer.
	 */
	header.dwBufferLength = size;
	header.lpData = block;
	/*
	 * prepare the block for playback
	 */
	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	/*
	 * write the block to the device. waveOutWrite returns immediately
	 * unless a synchronous driver is used (not often).
	 */
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));
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
	wfx.nChannels = 2; /* channels*/
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
	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) { 
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
	waveOutClose(hWaveOut);
}

void internal_audio_play_pcm(const audio_t* const data)
{
	writeAudioBlock((const LPSTR)data, sizeof(audio_t) * AUDIO_BUFFER_SIZE);
}


