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
	// TODO: implements
}


#endif
