#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"


#define FRAME_COUNTER_RATE     (CPU_FREQ / 240)
#define APU_SAMPLE_BUFFER_SIZE (37)
#define SOUND_BUFFER_SIZE      (2048)


static int_fast32_t cpuclk_last;
static int_fast16_t frame_cntdown;
static int_fast8_t frame_value;
static uint_fast8_t status;         // $4015
static uint_fast8_t frame_counter;  // $4017
static bool frame_irq;              // $4017
static bool dmc_irq;                // $4010
static bool apuclk_high;


const uint8_t length_tbl[0x20] = {
	10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30,
};

// Pulse channels
static struct Pulse {
	int_fast32_t period_cnt;
	int_fast32_t len_cnt;
	uint_fast8_t length;
	uint_fast16_t period;
	int_fast8_t duty_pos;
	uint_fast8_t duty;
	uint_fast8_t vol;
	uint_fast8_t output;
	bool enabled;
	bool length_enabled;
} pulse[2];

static const uint8_t duty_tbl[4][8] = {
	{ 0, 1, 0, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 0, 0, 0, 0, 0 },
	{ 0, 1, 1, 1, 1, 0, 0, 0 },
	{ 1, 0, 0, 1, 1, 1, 1, 1 }
};

static const double pulse_mix_tbl[31] = {
	0, 0.011609139523578026, 0.022939481268011527, 0.034000949216896059, 
	0.044803001876172609, 0.055354659248956883, 0.065664527956003665,
	0.075740824648844587, 0.085591397849462361, 0.095223748338502431, 
	0.10464504820333041, 0.11386215864759427, 0.12288164665523155,
	0.13170980059397538, 0.14035264483627205, 0.14881595346904861,
	0.15710526315789472, 0.16522588522588522, 0.17318291700241739,
	0.18098125249301955, 0.18862559241706162, 0.19612045365662886,
	0.20347017815646784, 0.21067894131185272, 0.21775075987841944, 
	0.22468949943545349, 0.23149888143176731, 0.23818248984115256,
	0.24474377745241579, 0.2511860718171926,  0.25751258087706685
};


static double pulse_samples[APU_SAMPLE_BUFFER_SIZE];
static int_fast8_t pulse_samples_index;
static int16_t sound_buffer[SOUND_BUFFER_SIZE];
static int_fast16_t sound_buffer_index;


static void update_pulse_output(struct Pulse* const p)
{
	if (!p->enabled || p->length == 0 || p->period_cnt < 8 ||
	    !duty_tbl[p->duty][p->duty_pos])
		p->output = 0;
	else
		p->output = p->vol;
}

static void steppulse(void)
{
	for (int n = 0; n < 2; ++n) {
		if (--pulse[n].period_cnt == 0) {
			pulse[n].period_cnt = pulse[n].period + 1;
			pulse[n].duty_pos = (pulse[n].duty_pos + 1) & 0x07;
			update_pulse_output(&pulse[n]);
		}
	}
}


static void step_frame_counter(void)
{
	switch (frame_counter) {
	case 4:
		frame_value = (frame_value + 1) % 4;
		switch (frame_value) {
		case 3: set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, frame_irq); break;
		}
		break;
	case 5:
		frame_value = (frame_value + 1) % 5;
		break;
	}
}

static void mixaudio(void)
{
	pulse_samples[pulse_samples_index++] = pulse_mix_tbl[pulse[0].output + pulse[1].output];

	if (pulse_samples_index >= APU_SAMPLE_BUFFER_SIZE) {
		double avg = 0;
		for (int i = 0; i < APU_SAMPLE_BUFFER_SIZE; ++i)
			avg += pulse_samples[i];
		avg /= APU_SAMPLE_BUFFER_SIZE;

		const int_fast16_t sample = -8000 + avg * 16000;
		sound_buffer[sound_buffer_index++] = sample;

		if (sound_buffer_index >= SOUND_BUFFER_SIZE) {
			playbuffer((uint8_t*)sound_buffer, sizeof(sound_buffer));
			sound_buffer_index = 0;
		}

		pulse_samples_index = 0;
	}
}

void resetapu(void)
{
	cpuclk_last = 0;
	apuclk_high = false;
	sound_buffer_index = 0;
	memset(sound_buffer, 0x00, sizeof(sound_buffer));


	status = 0;
	frame_cntdown = FRAME_COUNTER_RATE;
	frame_counter = 4;
	frame_value = 0;
	frame_irq = false;
	dmc_irq = false;

	// Pulse
	pulse_samples_index = 0;
	memset(pulse, 0x00, sizeof(pulse));
	for (int n = 0; n < 2; ++n)
		pulse[n].period_cnt = 1;
}

void stepapu(void)
{
	extern const int_fast32_t cpuclk;

	const int_fast32_t ticks = cpuclk >= cpuclk_last ? cpuclk - cpuclk_last : cpuclk;
	cpuclk_last = cpuclk;

	for (int_fast32_t i = 0; i < ticks; ++i) {
		if (--frame_cntdown <= 0) {
			frame_cntdown += FRAME_COUNTER_RATE;
			step_frame_counter();
		}

		if (apuclk_high) {
			steppulse();
			// stepnoise()
			// stepdmc()
		}

		// steptriangle()

		mixaudio();

		apuclk_high = !apuclk_high;
	}
}



static void write_pulse_reg0(const uint_fast8_t val, struct Pulse* const p)
{
	p->duty = val>>6;
	p->vol  = val&0x0F;
	p->length_enabled = (val&0x20) == 0;
	update_pulse_output(p);
}

static void write_pulse_reg2(const uint_fast8_t val, struct Pulse* const p)
{
	p->period = (p->period&0x0F00)|val;
	update_pulse_output(p);
}

static void write_pulse_reg3(const uint_fast8_t val, struct Pulse* const p)
{
	p->period = (p->period&0x00FF)|((val&0x07)<<8);
	p->length = length_tbl[(val&0xF8)>>3];
	p->duty_pos = 0;
	update_pulse_output(p);
}

static void write_dmc_reg0(const uint_fast8_t val)
{
	set_irq_source(IRQ_SRC_APU_DMC_TIMER, (val&0x80) != 0);
}

static void write_apu_status(const uint_fast8_t val)
{
	for (int n = 0; n < 2; ++n) {
		if (!(pulse[n].enabled = val&(1<<n))) {
			pulse[n].len_cnt = 0;
			update_pulse_output(&pulse[n]);
		}
	}
}

static void write_frame_counter(const uint_fast8_t val)
{
	frame_counter = 4 + ((val>>7)&1);
	frame_irq = (val&0x40) == 0;
}

static uint_fast8_t read_apu_status(void)
{
	const uint_fast8_t ret =
		(dmc_irq == true ? 0x80 : 0x00) |
		(frame_irq == true ? 0x40 : 0x00) |
		((pulse[1].len_cnt > 0)<<1) |
		(pulse[0].len_cnt > 0);

	frame_irq = false;
	return ret;
}


void apuwrite(const uint_fast8_t val, const int_fast32_t addr)
{
	switch (addr) {
	case 0x4000: write_pulse_reg0(val, &pulse[0]); break;
	case 0x4002: write_pulse_reg2(val, &pulse[0]); break;
	case 0x4003: write_pulse_reg3(val, &pulse[0]); break;
	
	case 0x4004: write_pulse_reg0(val, &pulse[1]); break;
	case 0x4006: write_pulse_reg2(val, &pulse[1]); break;
	case 0x4007: write_pulse_reg3(val, &pulse[1]); break;
	
	case 0x4010: write_dmc_reg0(val);      break;
	case 0x4015: write_apu_status(val);    break;
	case 0x4017: write_frame_counter(val); break;
	}
}

uint_fast8_t apuread(const int_fast32_t addr)
{
	switch (addr) {
	case 0x4015: return read_apu_status();
	}

	return 0;
}

