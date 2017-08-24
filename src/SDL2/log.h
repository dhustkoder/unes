#ifndef UNES_LOG_H_
#define UNES_LOG_H_
#include "SDL.h"


#define loginfo(...)  SDL_Log(__VA_ARGS__)
#define logerror(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#ifdef DEBUG
#define logdebug(...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#else
#define logdebug(...) ((void)0)
#endif

#endif
