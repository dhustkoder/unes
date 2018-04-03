#ifndef UNES_AUDIO_H_
#define UNES_AUDIO_H_
#include <stdint.h>


#define AUDIO_MAX_VOLUME  (0)
#define AUDIO_FREQUENCY   (48000)
#define AUDIO_BUFFER_SIZE (2048)


typedef int16_t audio_t;


static inline void queue_audio_buffer(const audio_t* const buffer)
{
	((void)buffer);
}


#endif
