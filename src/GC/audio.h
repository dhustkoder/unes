#ifndef UNES_AUDIO_H_
#define UNES_AUDIO_H_
#include <stdint.h>


#define AUDIO_MAX_VOLUME (0)


static void queue_sound_buffer(const uint8_t* const buffer, const uint_fast32_t len)
{
	((void)buffer);
	((void)len);
}


#endif
