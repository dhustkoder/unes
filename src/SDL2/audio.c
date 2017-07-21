#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "audio.h"


#define SAMPLE_RATE  (44100)


void audio_callback();

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

	return true;
}

void termaudio(void)
{
	SDL_CloseAudio();
}

void audio_callback()
{
}

