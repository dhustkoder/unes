#include <stdint.h>
#include <SDL2/SDL.h>
#include "audio.h"

extern uint8_t io[0x28];

#define P1_CTRL (io[0x08])
#define P1_RAMP (io[0x09])
#define P1_FT   (io[0x0A])
#define P1_CT   (io[0x0B])

static struct {
	uint_fast16_t timer;

	uint_fast8_t duty;
	uint_fast8_t len_halt;
	uint_fast8_t constvol;
	uint_fast8_t volume;

	uint_fast8_t sweep_enabled;
	uint_fast8_t period;
	uint_fast8_t negate;
	uint_fast8_t shift;

	uint_fast8_t len_load;
} p1;


void stepapu(void)
{
	p1.duty = (P1_CTRL&0xC0)>>6;
	p1.len_halt = (P1_CTRL&0x20)>>5;
	p1.constvol = (P1_CTRL&0x10)>>4;
	p1.volume = P1_CTRL&0x0F;

	p1.sweep_enabled = (P1_RAMP&0x80)>>7;
	p1.period = (P1_RAMP&0x70)>>4;
	p1.negate = (P1_RAMP&0x08)>>3;
	p1.shift  = (P1_RAMP&0x07);

	p1.timer  = ((P1_CT&0x07)<<4)|P1_FT;
	p1.len_load = (P1_CT&0xF8)>>3;
}


