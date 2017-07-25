#ifndef UNES_SDL2_AUDIO_H_
#define UNES_SDL2_AUDIO_H_
#include <stdint.h>
#include <stdbool.h>

extern bool initaudio(void);
extern void termaudio(void);
extern void playbuffer(const uint8_t* buffer, const uint_fast32_t len);

#endif
