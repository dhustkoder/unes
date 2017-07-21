#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#define M_PI (3.14159265359)
const int AMPLITUDE = 28000;
const int SAMPLE_RATE = 44100;

void audio_callback(void *user_data, Uint8 *raw_buffer, int bytes)
{
	Sint16 *buffer = (Sint16*)raw_buffer;
	int length = bytes / 2; // 2 bytes per sample for AUDIO_S16SYS
	int sample_nr = (*(int*)user_data);

	for(int i = 0; i < length; i++, sample_nr++) {
		double time = (double)sample_nr / (double)SAMPLE_RATE;
		buffer[i] = (Sint16)(AMPLITUDE * sin(2.0f * M_PI * 441.0f * time)); // render 441 HZ sine wave
	}
}


int main(int argc, char *argv[])
{
	if(SDL_Init(SDL_INIT_AUDIO) != 0) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	int sample_nr = 0;

	SDL_AudioSpec want;
	want.freq = SAMPLE_RATE; // number of samples per second
	want.format = AUDIO_S16SYS; // sample type (here: signed short i.e. 16 bit)
	want.channels = 1; // only one channel
	want.samples = 2048; // buffer-size
	want.callback = audio_callback; // function SDL calls periodically to refill the buffer
	want.userdata = &sample_nr; // counter, keeping track of current sample number

	SDL_AudioSpec have;
	if(SDL_OpenAudio(&want, &have) != 0)
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open audio: %s", SDL_GetError());
	if(want.format != have.format)
		SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to get the desired AudioSpec");

	SDL_PauseAudio(0); // start playing sound
	SDL_Delay(1000); // wait while sound is playing
	SDL_PauseAudio(1); // stop playing sound

	SDL_CloseAudio();
}



