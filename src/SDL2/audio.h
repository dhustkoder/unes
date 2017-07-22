#ifndef UNES_SDL2_AUDIO_H_
#define UNES_SDL2_AUDIO_H_
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL_audio.h>

extern bool initaudio(void);
extern void termaudio(void);
extern void playbuffer(const int16_t* buffer);

#endif
