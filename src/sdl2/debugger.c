#include <stdbool.h>
#include <SDL.h>
#include "log.h"
#include "debugger.h"

#ifdef UNES_DEBUGGER
static SDL_atomic_t thread_state;
enum thread_states {
	THREAD_RUNNING,
	THREAD_KILL,
	THREAD_TERM
};


static SDL_Thread* dbg_thread;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* sdl_texture;


static int dbg_thread_main(void* data)
{
	((void)data);


	SDL_AtomicSet(&thread_state, THREAD_RUNNING);

	while (SDL_AtomicGet(&thread_state) == THREAD_RUNNING) {
		SDL_RenderClear(renderer);
		SDL_RenderPresent(renderer);
		SDL_Delay(1000 / 60);
	}

	SDL_AtomicSet(&thread_state, THREAD_TERM);
	return 0;
}


bool initialize_debugger()
{
	// video
	window = SDL_CreateWindow("µnes debugger", SDL_WINDOWPOS_CENTERED,
				  SDL_WINDOWPOS_CENTERED,
				  UNES_DEBUGGER_SCR_WIDTH, UNES_DEBUGGER_SCR_HEIGHT,
				  SDL_WINDOW_RESIZABLE);
	if (window == NULL) {
		log_error("Failed to create SDL_Window: %s\n", SDL_GetError());
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1,
	                              SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		log_error("Failed to create SDL_Renderer: %s\n", SDL_GetError());
		goto Lfreewindow;
	}

	SDL_RendererInfo info;
	SDL_GetRendererInfo(renderer, &info);
	sdl_texture = SDL_CreateTexture(renderer, info.texture_formats[0],
	                            SDL_TEXTUREACCESS_STREAMING,
	                            UNES_DEBUGGER_SCR_WIDTH,
				    UNES_DEBUGGER_SCR_HEIGHT);
	if (sdl_texture == NULL) {
		log_error("Failed to create SDL_Texture: %s\n", SDL_GetError());
		goto Lfreerenderer;
	}

	dbg_thread = SDL_CreateThread(
		dbg_thread_main,
		"dbg_thread",
		NULL
	);

	return true;

Lfreerenderer:
	SDL_DestroyRenderer(renderer);
Lfreewindow:
	SDL_DestroyWindow(window);

	return false;


}


void terminate_debugger()
{
	SDL_AtomicSet(&thread_state, THREAD_KILL);
	while (SDL_AtomicGet(&thread_state) != THREAD_TERM) {
		SDL_Delay(1);
	}

	SDL_DestroyTexture(sdl_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}





#endif












