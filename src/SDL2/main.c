#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "audio.h"


extern int unes(int argc,  char* argv[]);


int main(int argc, char *argv[])
{
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	} else if (!initaudio()) {
		return EXIT_FAILURE;
	}

	const int exitcode = unes(argc, argv);

	termaudio();
	SDL_Quit();
	return exitcode;
}



