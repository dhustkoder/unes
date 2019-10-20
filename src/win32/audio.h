#ifndef UNES_AUDIO_H_
#define UNES_AUDIO_H_
#include <stdint.h>


#define AUDIO_MAX_VOLUME (100)
#define AUDIO_FREQUENCY (44100)
#define AUDIO_BUFFER_SIZE (1024 * 6)

typedef int16_t audio_t;


static inline void queue_audio_buffer(const audio_t* const buffer)
{
	extern void internal_audio_play_pcm(const audio_t* data);
	internal_audio_play_pcm(buffer);	
}


#endif
