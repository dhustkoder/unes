#ifndef UNES_PLATFORM_H_
#define UNES_PLATFORM_H_
#include <Windows.h>
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

typedef enum {
	LOGGER_ID_STDOUT,
	LOGGER_ID_STDERR,
	LOGGER_ID_COUNT
} LoggerID;


extern void internal_logger(LoggerID id, const char* fmt, ...);
#define log_info(fmtstr, ...) internal_logger(LOGGER_ID_STDOUT, fmtstr, __VA_ARGS__)
#define log_error(fmtstr, ...) internal_logger(LOGGER_ID_STDERR, fmtstr, __VA_ARGS__)
#ifdef DEBUG
#define log_debug(fmtstr, ...) internal_logger(LOGGER_ID_STDOUT, fmtstr, __VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif



extern void video_internal_render(const uint8_t* fb);
extern void internal_audio_push_buffer(const audio_sample_t* buf);

#define queue_video_buffer(fb) video_internal_render(fb)
#define queue_audio_buffer(buf) internal_audio_push_buffer(buf)



#endif
