#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>


extern int unes(const int argc, const char* const* const argv);


int main(const int argc, const char* const* const argv)
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "%s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	return unes(argc, argv);
}


