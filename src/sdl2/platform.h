#ifndef UNES_PLATFORM_H_
#define UNES_PLATFORM_H_
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <assert.h>



typedef int16_t audio_sample_t;

#define AUDIO_SAMPLE_SIZE         (sizeof(audio_sample_t))
#define AUDIO_SAMPLES_PER_SEC     (44100)
#define AUDIO_BUFFER_SAMPLE_COUNT (735)
#define AUDIO_BUFFER_SIZE         (AUDIO_BUFFER_SAMPLE_COUNT * AUDIO_SAMPLE_SIZE)
#define AUDIO_CHANNEL_COUNT       (1)


#define log_info(...)  SDL_Log(__VA_ARGS__)
#define log_error(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#ifdef DEBUG
#define log_debug(...) SDL_Log(__VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif

extern void queue_audio_buffer(const void* fb);
extern void queue_video_buffer(const void* fb);







#endif
