#ifndef UNES_SDL2_AUDIO_H_
#define UNES_SDL2_AUDIO_H_
#include <stdint.h>
#include "SDL.h"
#include "SDL_audio.h"


#define AUDIO_MAX_VOLUME (SDL_MIX_MAXVOLUME)


static inline void mixsample(int16_t* const dest_sample, int16_t src_sample, int volume)
{
	SDL_MixAudioFormat((uint8_t*)dest_sample, (uint8_t*)&src_sample, AUDIO_S16SYS,
	                   sizeof(int16_t), volume);
}

static inline void queue_sound_buffer(const uint8_t* const buffer, const uint_fast32_t len)
{
	extern SDL_AudioDeviceID audio_device;

	while (SDL_GetQueuedAudioSize(audio_device) > len)
		SDL_Delay(1);

	if (SDL_QueueAudio(audio_device, buffer, len) != 0)
		fprintf(stderr, "Failed to Queue audio: %s", SDL_GetError());
}


#endif
