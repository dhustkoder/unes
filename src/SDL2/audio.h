#ifndef UNES_AUDIO_H_
#define UNES_AUDIO_H_
#include <stdint.h>
#include "SDL.h"
#include "SDL_audio.h"


#define AUDIO_MAX_VOLUME (SDL_MIX_MAXVOLUME)


static void queue_sound_buffer(const uint8_t* const buffer, const uint_fast32_t len)
{
	extern SDL_AudioDeviceID audio_device;
	while (SDL_GetQueuedAudioSize(audio_device) > len)
		SDL_Delay(1);
	SDL_QueueAudio(audio_device, buffer, len);
}


#endif
