#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "audio.h"


#define SAMPLE_RATE  (44100)

static int16_t sndbuffer[1024];

static void audio_callback(void* const userdata, uint8_t* const buffer, const int buflen)
{
	((void)userdata);
	int16_t* const dst = (int16_t*)buffer;
	const int len = buflen / sizeof(int16_t);

	for (int i = 0; i < len; ++i)
		dst[i] = sndbuffer[i];
}

bool initaudio(void)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = SAMPLE_RATE;        // number of samples per second
	want.format = AUDIO_S16SYS;     // sample type (here: signed short i.e. 16 bit)
	want.channels = 1;              // only one channel
	want.samples = 2048;            // buffer-size
	want.callback = audio_callback; // function SDL calls periodically to refill the buffer

	SDL_AudioSpec have;

	if (SDL_OpenAudio(&want, &have) != 0) {	
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
		return false;
	} else if (want.format != have.format) {
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");
		return false;
	}

	SDL_PauseAudio(0);
	return true;
}

void termaudio(void)
{
	SDL_CloseAudio();
}

void playbuffer(const int16_t* const buffer)
{
	SDL_LockAudio();
	memcpy(sndbuffer, buffer, sizeof(sndbuffer));
	SDL_UnlockAudio();
}

