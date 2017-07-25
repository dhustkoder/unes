#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "audio.h"


static SDL_AudioDeviceID audio_device;


bool initaudio(void)
{
	SDL_AudioSpec want;
	SDL_zero(want);
	want.freq = 48000;
	want.format = AUDIO_S16SYS;
	want.channels = 1;
	want.samples = 2048;

	if ((audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0)) == 0) {
		fprintf(stderr, "Failed to open audio: %s", SDL_GetError());
		return false;
	}

	SDL_PauseAudioDevice(audio_device, 0);
	return true;
}

void termaudio(void)
{
	SDL_CloseAudioDevice(audio_device);
}

void playbuffer(const uint8_t* const buffer, const uint_fast32_t len)
{
	if (SDL_QueueAudio(audio_device, buffer, len) != 0)
		fprintf(stderr, "Failed to Queue audio: %s", SDL_GetError());
}

