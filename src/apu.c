#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"

int_fast32_t apuclk;

static struct Pulse {
	uint_fast16_t timer;
	uint_fast8_t duty;
	uint_fast8_t vol;
} p1;

static const uint_fast8_t duties[4] = { 0x40, 0x60, 0x78, 0x9F };
static uint8_t output[100];

void resetapu(void)
{
	apuclk = 0;
	memset(&p1, 0x00, sizeof(p1));
}


void stepapu(void)
{

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
