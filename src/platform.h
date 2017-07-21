#ifndef UNES_PLATFORM_H_
#define UNES_PLATFORM_H_


#ifdef PLATFORM_SDL2
#include <stdint.h>
#include <SDL2/SDL.h>


// platform utils
#define gettime()      ((uint_fast32_t)SDL_GetTicks())
#define delay(milli)   (SDL_Delay(milli))



#else
#error "No platform specified."
#endif









#endif
