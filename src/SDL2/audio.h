#ifndef UNES_SDL2_AUDIO_H_
#define UNES_SDL2_AUDIO_H_
#include <stdint.h>
#include "SDL.h"
#include "SDL_audio.h"


#define AUDIO_MAX_VOLUME (SDL_MIX_MAXVOLUME)


static inline void queue_sound_buffer(const uint8_t* const buffer, const uint_fast32_t len)
{
	extern SDL_AudioDeviceID audio_device;
	SDL_QueueAudio(audio_device, buffer, len);
}


#endif
