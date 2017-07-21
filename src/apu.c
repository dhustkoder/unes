#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"

int_fast32_t apuclk;

static struct Pulse {
	uint_fast16_t timer;
	uint_fast16_t timer_cnt;
	uint_fast8_t duty;
	uint_fast8_t vol;
} p1;

static const uint_fast8_t duties[4] = { 0x40, 0x60, 0x78, 0x9F };
static uint8_t output[100];
static uint8_t output_idx;

void resetapu(void)
{
	apuclk = 0;
	output_idx = 0;
	memset(&p1, 0x00, sizeof(p1));
	memset(output, 0x00, sizeof(output));
}


void stepapu(void)
{
	extern const int_fast32_t cpuclk;
	do {
		output[output_idx] = (duties[p1.duty]>>(output_idx%8));
		--p1.timer_cnt;
		if (p1.timer_cnt == 0) {
			p1.timer_cnt = p1.timer;
			++output_idx;
			if (output_idx == 100) {
				output_idx = 0;
				playaudio(output);
			}
		}
	} while (++apuclk < cpuclk);
}


void apuwrite(const uint_fast8_t val, const int_fast32_t addr)
{
	switch (addr) {
	case 0x4000:
		p1.duty = (val&0xC0)>>6;
		p1.vol  = val&0x0F;
		break;
	case 0x4002:
		p1.timer = (p1.timer&0x0700)|val;
		break;
	case 0x4003:
		p1.timer = (p1.timer&0x00FF)|(((uint_fast16_t)val)<<8);
		p1.timer &= 0x07FF;
		break;
	}
}


uint_fast8_t apuread(const int_fast32_t addr)
{
	switch (addr) {
	case 0x4000:
		break;
	case 0x4002:
		break;
	case 0x4003:
		break;
	}
	return 0;
}
