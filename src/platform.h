#ifndef UNES_PLATFORM_H_
#define UNES_PLATFORM_H_
#include <stdint.h>

#ifdef PLATFORM_SDL2
#include <SDL/SDL.h>
#define gettime()      ((uint_fast32_t)SDL_GetTicks())
#define delay(milli)   (SDL_Delay(milli))
#else
#error "No platform specified."
#endif









#endif
