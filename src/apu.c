#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"


int_fast32_t apuclk;
static uint_fast8_t status;


// Pulse channels

static struct Pulse {
	int_fast32_t period_cnt;
	int_fast32_t len_cnt;
	uint_fast16_t period;
	int_fast8_t waveform_pos;
	uint_fast8_t duty;
	uint_fast8_t vol;
	uint_fast8_t output;
	bool enabled;
} pulse[2];

static const uint8_t pulse_duties[4][8] = {
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 }
};


static double pulse_mixer_table[31];
static double pulse_samples[40];
static int_fast8_t pulse_samples_index;
static int16_t sound_buffer[1024];
static int_fast16_t sound_buffer_index;


static void update_pulse_output(const uint_fast8_t n)
{
	if (!pulse[n].enabled ||
	    pulse[n].period_cnt < 8 ||
	    !pulse_duties[pulse[n].duty][pulse[n].waveform_pos])
		pulse[n].output = 0;
	else
		pulse[n].output = pulse[n].vol;
}

static void steppulse(void)
{
	for (int n = 0; n < 2; ++n) {
		if (--pulse[n].period_cnt == 0) {
			pulse[n].period_cnt = pulse[n].period + 1;
			pulse[n].waveform_pos = (pulse[n].waveform_pos + 1) & 0x07;
			update_pulse_output(n);
		}
	}
}

static void mixaudio(void)
{
	pulse_samples[pulse_samples_index++] = pulse_mixer_table[pulse[0].output + pulse[1].output];
	const int numsamples = (int)sizeof(pulse_samples)/sizeof(pulse_samples[0]);

	if (pulse_samples_index >= numsamples) {
		double avg = 0;
		for (int i = 0; i < numsamples; ++i)
			avg += pulse_samples[i];
		avg /= (double)numsamples;

		const int_fast16_t sample = INT16_MIN + avg * (INT16_MAX - INT16_MIN);

		sound_buffer[sound_buffer_index++] = sample;

		if (sound_buffer_index >= (int)(sizeof(sound_buffer)/sizeof(sound_buffer[0]))) {
			playbuffer((uint8_t*)sound_buffer, sizeof(sound_buffer));
			sound_buffer_index = 0;
		}

		pulse_samples_index = 0;
	}
}



void resetapu(void)
{
	apuclk = 0;
	status = 0;
	sound_buffer_index = 0;
	pulse_samples_index = 0;
	memset(sound_buffer, 0x00, sizeof(sound_buffer));

	// Pulse channels init
	memset(pulse, 0x00, sizeof(pulse));

	for (int n = 0; n < 2; ++n)
		pulse[n].period_cnt = 1;
	for (int n = 0; n < 31; ++n)
		pulse_mixer_table[n] = 95.52 / (8128.0 / n + 100.0);
}

void stepapu(void)
{
	extern const int_fast32_t cpuclk;

	do {
		steppulse();
		mixaudio();
	} while (++apuclk <= cpuclk);
}



static void write_pulse_reg0(const uint_fast8_t n, const uint_fast8_t val)
{
	pulse[n].duty = val>>6;
	pulse[n].vol  = val&0x0F;
	update_pulse_output(n);
}

static void write_pulse_reg2(const uint_fast8_t n, uint_fast8_t val)
{
	pulse[n].period = (pulse[n].period&0x0F00)|val;
	update_pulse_output(n);
}

static void write_pulse_reg3(const uint_fast8_t n, uint_fast8_t val)
{
	pulse[n].period = (pulse[n].period&0x00FF)|((val&0x07)<<8);
	update_pulse_output(n);
}

static void write_apu_status(const uint_fast8_t val)
{
	for (int n = 0; n < 2; ++n) {
		if (!(pulse[n].enabled = val&(1<<n))) {
			pulse[n].len_cnt = 0;
			update_pulse_output(n);
		}
	}
}

static uint_fast8_t read_apu_status(void)
{
	return ((pulse[1].len_cnt > 0)<<1) |
	       (pulse[0].len_cnt > 0);
}


void apuwrite(const uint_fast8_t val, const int_fast32_t addr)
{
	switch (addr) {
	case 0x4000: write_pulse_reg0(0, val); break;
	case 0x4002: write_pulse_reg2(0, val); break;
	case 0x4003: write_pulse_reg3(0, val); break;
	case 0x4015: write_apu_status(val);    break;
	}
}

uint_fast8_t apuread(const int_fast32_t addr)
{
	switch (addr) {
	case 0x4015: return read_apu_status();
	}

	return 0;
}

