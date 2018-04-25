#ifndef UNES_AUDIO_H_
#define UNES_AUDIO_H_
#include <stdint.h>
#include "SDL.h"
#include "SDL_audio.h"


#define AUDIO_MAX_VOLUME (SDL_MIX_MAXVOLUME)
#define AUDIO_BUFFER_SIZE (1024)
#define AUDIO_FREQUENCY (44100)


typedef int16_t audio_t;


static inline void queue_audio_buffer(const audio_t* const buffer)
{
	extern SDL_AudioDeviceID audio_device;

	const Uint32 size = sizeof(*buffer) * AUDIO_BUFFER_SIZE;
	SDL_QueueAudio(audio_device, buffer, size);
	while (SDL_GetQueuedAudioSize(audio_device) > size) {
		//...
	}
}


#endif
