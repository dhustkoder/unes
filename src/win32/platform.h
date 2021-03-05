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


#define AUDIO_MAX_VOLUME   (100)
#define AUDIO_FREQUENCY    (44100)
#define AUDIO_BUFFER_SIZE  (2048)

typedef int16_t audio_t;



enum stdhandle_index {
	UNES_LOG_STDOUT,
	UNES_LOG_STDERR
};



#define log_info(fmtstr, ...) internal_logger(UNES_LOG_STDOUT, fmtstr, __VA_ARGS__)
#define log_error(fmtstr, ...) internal_logger(UNES_LOG_STDERR, fmtstr, __VA_ARGS__)
#ifdef DEBUG
#define log_debug(fmtstr, ...) internal_logger(UNES_LOG_STDOUT, fmtstr, __VA_ARGS__)
#else
#define log_debug(...) ((void)0)
#endif


extern void init_log_system(void);
extern void term_log_system(void);
extern void internal_logger(enum stdhandle_index idx, const char* fmt, ...);


extern BOOL init_video_system(HINSTANCE hInstance, int nCmdShow);
extern BOOL video_win_update(void);
extern void video_start_frame(void);
extern void video_end_frame(void);
extern void term_video_system(void);

extern BOOL init_audio_system(void);
extern void term_audio_system(void);
extern void internal_audio_sync(void);

extern void video_internal_render(const uint8_t* fb);
#define render(fb) video_internal_render(fb)

extern void internal_audio_push_buffer(const audio_t* buf);
#define queue_audio_buffer(buf) internal_audio_push_buffer(buf)



#endif
