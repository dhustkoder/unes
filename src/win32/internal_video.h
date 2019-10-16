#ifndef INTERNAL_VIDEO_H_
#define INTERNAL_VIDEO_H_
#include <Windows.h>
#include "video.h"

extern BOOL init_video_system(HINSTANCE hInstance);
extern BOOL video_win_update(void);
extern void video_start_frame(void);
extern void video_end_frame(void);
extern void term_video_system(void);


#endif