#include <stdint.h>
#include <string.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"


#define FRAME_COUNTER_RATE     (CPU_FREQ / 240)
#define APU_SAMPLE_BUFFER_SIZE (CPU_FREQ / 48000)
#define SOUND_BUFFER_SIZE      (2048)


static int_fast32_t frame_counter_clock;
static int_fast8_t delayed_frame_timer_reset;
static uint_fast8_t status;         // $4015
static uint_fast8_t frame_counter;  // $4017
static bool irq_inhibit;            // $4017
static bool dmc_irq;                // $4010
static bool oddtick;

static int16_t sound_buffer[SOUND_BUFFER_SIZE];
static int_fast16_t sound_buffer_idx;
static double apu_samples[APU_SAMPLE_BUFFER_SIZE];
static int_fast8_t apu_samples_idx;

// Pulse channels
static struct Pulse {
	int_fast32_t period_cnt;
	int_fast32_t len_cnt;
	int_fast16_t env_cnt;
	int_fast16_t env_vol;
	uint_fast16_t period;
	int_fast8_t duty_pos;
	uint_fast8_t duty_mode;
	uint_fast8_t vol;
	uint_fast8_t out;
	uint_fast8_t sweep_period;
	uint_fast8_t sweep_shift;
	bool enabled;
	bool const_vol;
	bool len_enabled;
	bool env_start;
	bool sweep_enabled;
	bool sweep_negate;
	bool sweep_reload;
} pulse[2];


static void update_pulse_out(struct Pulse* const p)
{
	static const uint8_t duty_tbl[4][8] = {
		{ 0, 1, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 0, 0, 0, 0, 0 },
		{ 0, 1, 1, 1, 1, 0, 0, 0 },
		{ 1, 0, 0, 1, 1, 1, 1, 1 }
	};

	if (!p->enabled || p->len_cnt == 0 ||
	    p->period_cnt < 8 || p->period_cnt > 0x7FF || 
	    !duty_tbl[p->duty_mode][p->duty_pos])
		p->out = 0;
	else
		p->out = p->const_vol ? p->vol : p->env_vol;
}

static void step_pulse_timer(struct Pulse* const p)
{
	if (--p->period_cnt == 0) {
		p->period_cnt = p->period + 1;
		p->duty_pos = (p->duty_pos + 1)&0x07;
	}
}

static void step_pulse_envelope(struct Pulse* const p)
{
	if (p->env_start) {
		p->env_start = false;
		p->env_vol = 0x0F;
		p->env_cnt = p->vol;
	} else if (p->env_cnt-- < 0) {
		p->env_cnt = p->vol;
		if (p->env_vol > 0)
			--p->env_vol;
		else if (!p->len_enabled)
			p->env_vol = 0x0F;
	}
}

static void step_pulse_length(struct Pulse* const p)
{
	if (p->len_enabled && p->len_cnt > 0)
		--p->len_cnt;
}

static void step_timers(void)
{
	if (!oddtick) {
		step_pulse_timer(&pulse[0]);
		step_pulse_timer(&pulse[1]);
	}
}

static void step_envelopes(void)
{
	step_pulse_envelope(&pulse[0]);
	step_pulse_envelope(&pulse[1]);
}

static void step_lengths(void)
{
	step_pulse_length(&pulse[0]);
	step_pulse_length(&pulse[1]);
}

static void step_frame_counter(void)
{
	// thanks to nesalizer
	#define T1 (3728)
	#define T2 (7456)
	#define T3 (11185)
	#define T4 (14914)
	#define T5 (18640)

	if (!oddtick)
		return;

	switch (frame_counter) {
	case 0:
		if (delayed_frame_timer_reset > 0 && --delayed_frame_timer_reset == 0) {
			frame_counter_clock = 0;
		} else if (++frame_counter_clock == T4 + 2) {
			frame_counter_clock = 0;
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
		}

		switch (frame_counter_clock) {
		case T1 + 1: case T3 + 1:
			step_envelopes();
			break;

		case T2 + 1:
			step_lengths();
			step_envelopes();
			break;

		case T4:
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
			break;

		case T4 + 1:
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
			step_lengths();
			step_envelopes();
			break;
		}
		break;

	case 1:
		if (delayed_frame_timer_reset > 0 && --delayed_frame_timer_reset == 0) {
			frame_counter_clock = 0;
		} else if (++frame_counter_clock == T5 + 2) {
			frame_counter_clock = 0;
		}

		switch (frame_counter_clock) {
		case T2 + 1: case T5 + 1:
			step_lengths();
			step_envelopes();
			break;

		case T1 + 1: case T3 + 1:
			step_envelopes();
			break;
		}
		break;
	}
}

static void update_outputs(void)
{
	update_pulse_out(&pulse[0]);
	update_pulse_out(&pulse[1]);
}

static void mixaudio(void)
{
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

	update_outputs();
	const double pulse_sample = pulse_mix_tbl[pulse[0].out + pulse[1].out];
	apu_samples[apu_samples_idx++] = pulse_sample;

	if (apu_samples_idx >= APU_SAMPLE_BUFFER_SIZE) {
		double avg = 0;
		for (int i = 0; i < APU_SAMPLE_BUFFER_SIZE; ++i)
			avg += apu_samples[i];
		avg /= APU_SAMPLE_BUFFER_SIZE;

		const int_fast16_t sample = avg * INT16_MAX;
		sound_buffer[sound_buffer_idx++] = sample;

		if (sound_buffer_idx >= SOUND_BUFFER_SIZE) {
			playbuffer((uint8_t*)sound_buffer, sizeof(sound_buffer));
			sound_buffer_idx = 0;
		}

		apu_samples_idx = 0;
	}
}

void resetapu(void)
{
	status = 0;
	delayed_frame_timer_reset = 0;
	frame_counter_clock = 0;
	frame_counter = 4;
	irq_inhibit = false;
	dmc_irq = false;
	oddtick = false;

	// sound buffer, apu sample buffer
	sound_buffer_idx = 0;
	apu_samples_idx = 0;
	memset(sound_buffer, 0x00, sizeof(sound_buffer));
	memset(apu_samples, 0x00, sizeof(apu_samples));

	// Pulse
	memset(pulse, 0x00, sizeof(pulse));
	for (int n = 0; n < 2; ++n)
		pulse[n].period_cnt = 1;
}

void stepapu(const int_fast32_t aputicks)
{
	for (int_fast32_t i = 0; i < aputicks; ++i) {
		step_frame_counter();
		step_timers();
		mixaudio();
		oddtick = !oddtick;
	}
}



static void write_pulse_reg0(const uint_fast8_t val, struct Pulse* const p)
{
	p->duty_mode = val>>6;
	p->len_enabled = (val&0x20) == 0;
	p->const_vol = (val&0x10) != 0;
	p->vol = val&0x0F;
	p->env_start = true;
}

static void write_pulse_reg1(const uint_fast8_t val, struct Pulse* const p)
{
	p->sweep_enabled = (val&0x80) != 0;
	p->sweep_period = (val&0x70) >> 4;
	p->sweep_negate = (val&0x08) != 0;
	p->sweep_shift = val&0x07;
	p->sweep_reload = true;
}

static void write_pulse_reg2(const uint_fast8_t val, struct Pulse* const p)
{
	p->period = (p->period&0x0F00)|val;
}

static void write_pulse_reg3(const uint_fast8_t val, struct Pulse* const p)
{
	static const uint8_t length_tbl[0x20] = {
		10, 254, 20, 2, 40, 4, 80, 6,
		160, 8, 60, 10, 14, 12, 26, 14,
		12, 16, 24, 18, 48, 20, 96, 22,
		192, 24, 72, 26, 16, 28, 32, 30,
	};

	p->period = (p->period&0x00FF)|((val&0x07)<<8);
	if (p->enabled)
		p->len_cnt = length_tbl[val>>3];
	p->duty_pos = 0;
	p->env_start = true;
}

static void write_dmc_reg0(const uint_fast8_t val)
{
	set_irq_source(IRQ_SRC_APU_DMC_TIMER, (val&0x80) != 0);
}

static void write_apu_status(const uint_fast8_t val)
{
	for (int i = 0; i < 2; ++i) {
		pulse[i].enabled = (val&(1<<i)) != 0;
		if (!pulse[i].enabled)
			pulse[i].len_cnt = 0;
	}
	dmc_irq = false;
}

static void write_frame_counter(const uint_fast8_t val)
{
	frame_counter = val>>7;
	irq_inhibit = (val&0x40) != 0;
	
	if (irq_inhibit)	
		set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, false);

	delayed_frame_timer_reset = 2;

	if (frame_counter == 1) {
		step_envelopes();
		step_lengths();
	}
}

static uint_fast8_t read_apu_status(void)
{
	const bool frame_irq = get_irq_source(IRQ_SRC_APU_FRAME_COUNTER);

	const uint_fast8_t ret =
		(dmc_irq<<7)                |
		(frame_irq<<6)              |
		((pulse[1].len_cnt > 0)<<1) |
		(pulse[0].len_cnt > 0);

	set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, false);
	return ret;
}

void apuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	switch (addr) {
	case 0x4000: write_pulse_reg0(val, &pulse[0]); break;
	case 0x4001: write_pulse_reg1(val, &pulse[0]); break;
	case 0x4002: write_pulse_reg2(val, &pulse[0]); break;
	case 0x4003: write_pulse_reg3(val, &pulse[0]); break;
	
	case 0x4004: write_pulse_reg0(val, &pulse[1]); break;
	case 0x4005: write_pulse_reg1(val, &pulse[1]); break;
	case 0x4006: write_pulse_reg2(val, &pulse[1]); break;
	case 0x4007: write_pulse_reg3(val, &pulse[1]); break;
	
	case 0x4010: write_dmc_reg0(val);      break;
	case 0x4015: write_apu_status(val);    break;
	case 0x4017: write_frame_counter(val); break;
	}
}

uint_fast8_t apuread(const uint_fast16_t addr)
{
	switch (addr) {
	case 0x4015: return read_apu_status();
	}

	return 0;
}

