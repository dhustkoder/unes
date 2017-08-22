#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "audio.h"
#include "cpu.h"
#include "apu.h"


#define FRAME_COUNTER_RATE     (CPU_FREQ / 240)
#define APU_SAMPLES_CNT_LIMIT  (CPU_FREQ / 44100)
#define SOUND_BUFFER_SIZE      (2048)


static int32_t frame_counter_clock;
static int8_t delayed_frame_timer_reset;
static uint8_t status;                   // $4015
static uint8_t frame_counter_mode;       // $4017
static bool irq_inhibit;                 // $4017
static bool oddtick;

static int16_t sound_buffer[SOUND_BUFFER_SIZE];
static int16_t sound_buffer_idx;
static int8_t apu_samples_cnt;

// Pulse channels
static struct Pulse {
	int16_t len_cnt;             // -1 - 254
	int16_t timer;               //  0 - 2047  11 bits, should never be negative
	int16_t timer_cnt;           // -1 - 2047  11 bits, should count down to -1, then reload timer
	int16_t sweep_target;       
	int8_t duty_mode;            //  0 - 3
	int8_t duty_pos;             //  0 - 7
	uint8_t vol;                 //  0 - 15
	uint8_t out;                 //  0 - 15
	int8_t env_vol;              //  0 - 15
	int8_t env_cnt;              //  0 - 15
	uint8_t sweep_period;        //  0 - 7
	int8_t sweep_period_cnt;     //  0 - 7
	uint8_t sweep_shift;         //  0 - 7
	bool sweep_negate   : 1;
	bool sweep_enabled  : 1;
	bool sweep_reload   : 1;
	bool enabled        : 1;
	bool const_vol      : 1;
	bool len_enabled    : 1;
	bool env_start      : 1;
} pulse[2];


static void eval_sweep_target(struct Pulse* const p)
{
	const unsigned offset = p->timer >> p->sweep_shift;
	if (p->sweep_negate) {
		if (p == &pulse[0])
			p->sweep_target = p->timer - (offset + 1);
		else
			p->sweep_target = p->timer - offset;
		p->sweep_target &= 0x7FF;
	} else {
		p->sweep_target = p->timer + offset;
	}
}

static void write_pulse_reg0(const uint8_t val, struct Pulse* const p)
{
	p->duty_mode = val>>6;
	p->len_enabled = (val&0x20) == 0;
	p->const_vol = (val&0x10) != 0;
	p->vol = val&0x0F;
	p->env_start = true;
}

static void write_pulse_reg1(const uint8_t val, struct Pulse* const p)
{
	p->sweep_period = (val>>4)&0x07;
	p->sweep_negate = (val&0x08) != 0;
	p->sweep_shift = val&0x07;
	p->sweep_enabled = (val&0x80) != 0 && p->sweep_shift > 0;
	p->sweep_reload = true;
}

static void write_pulse_reg2(const uint8_t val, struct Pulse* const p)
{
	p->timer = (p->timer&0x0700)|val;
	eval_sweep_target(p);
}

static void write_pulse_reg3(const uint8_t val, struct Pulse* const p)
{
	const uint8_t length_tbl[0x20] = {
		10, 254, 20, 2, 40, 4, 80, 6,
		160, 8, 60, 10, 14, 12, 26, 14,
		12, 16, 24, 18, 48, 20, 96, 22,
		192, 24, 72, 26, 16, 28, 32, 30,
	};

	p->timer = (p->timer&0x00FF)|((val&0x07)<<8);
	if (p->enabled)
		p->len_cnt = length_tbl[val>>3];
	p->duty_pos = 0;
	p->env_start = true;
	eval_sweep_target(p);
}


static void write_dmc_reg0(const uint8_t val)
{
	set_irq_source(IRQ_SRC_APU_DMC_TIMER, (val&0x80) != 0);
}

static void tick_timer(void)
{
	if (!oddtick) {
		for (unsigned i = 0; i < 2; ++i) {
			if (--pulse[i].timer_cnt < 0) {
				pulse[i].timer_cnt = pulse[i].timer;
				pulse[i].duty_pos = (pulse[i].duty_pos + 1)&0x07;
			}
		}
	}
}

static void tick_envelope(void)
{
	for (unsigned i = 0; i < 2; ++i) {
		if (pulse[i].env_start) {
			pulse[i].env_start = false;
			pulse[i].env_vol = 15;
			pulse[i].env_cnt = pulse[i].vol;
		} else if (--pulse[i].env_cnt <= 0) {
			pulse[i].env_cnt = pulse[i].vol;
			if (pulse[i].env_vol > 0)
				--pulse[i].env_vol;
			else if (!pulse[i].len_enabled)
				pulse[i].env_vol = 15;
		}
	}
}

static void tick_length(void)
{
	for (unsigned i = 0; i < 2; ++i) {
		if (pulse[i].len_enabled && pulse[i].len_cnt > 0)
			--pulse[i].len_cnt;
	}
}

static void tick_sweep(void)
{
	for (unsigned i = 0; i < 2; ++i) {
		if (--pulse[i].sweep_period_cnt <= 0) {
			pulse[i].sweep_period_cnt = pulse[i].sweep_period;
			if (pulse[i].sweep_enabled && pulse[i].timer >= 8) {
				eval_sweep_target(&pulse[i]);
				if (pulse[i].sweep_negate || (pulse[i].sweep_target&0xF800) == 0)
					pulse[i].timer = pulse[i].sweep_target;
			}
		}

		if (pulse[i].sweep_reload) {
			pulse[i].sweep_reload = false;
			pulse[i].sweep_period_cnt = pulse[i].sweep_period;
		}
	}
}


static void tick_frame_counter(void)
{
	// thanks to nesalizer
	#define T1 (3728 * 2)
	#define T2 (7456 * 2)
	#define T3 (11185 * 2)
	#define T4 (14914 * 2)
	#define T5 (18640 * 2)


	switch (frame_counter_mode) {
	case 0:
		if (delayed_frame_timer_reset > 0 && --delayed_frame_timer_reset == 0) {
			frame_counter_clock = 0;
		} else if (++frame_counter_clock == T4 + 2) {
			frame_counter_clock = 0;
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
		}

		switch (frame_counter_clock) {
		case T1 + 1: case T3 + 1:
			tick_envelope();
			break;

		case T2 + 1:
			tick_length();
			tick_sweep();
			tick_envelope();
			break;

		case T4:
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
			break;

		case T4 + 1:
			set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, !irq_inhibit);
			tick_length();
			tick_sweep();
			tick_envelope();
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
			tick_length();
			tick_sweep();
			tick_envelope();
			break;

		case T1 + 1: case T3 + 1:
			tick_envelope();
			break;
		}
		break;
	}
}

static void write_frame_counter(const uint8_t val)
{
	frame_counter_mode = val>>7;
	irq_inhibit = (val&0x40) != 0;
	
	if (irq_inhibit)
		set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, false);

	// side effects
	delayed_frame_timer_reset = oddtick ? 4 : 3;

	if (frame_counter_mode == 1) {
		tick_length();
		tick_sweep();
		tick_envelope();
	}
}

static void update_channels_output(void)
{
	const uint8_t duty_tbl[4] = { 0x40, 0x60, 0x78, 0x9F };

	for (unsigned i = 0; i < 2; ++i) {
		struct Pulse* const p = &pulse[i];
		if (!p->enabled || p->len_cnt == 0 || p->timer < 8 || (p->timer&0xF800) ||
		    (!p->sweep_negate && (p->sweep_target&0xF800) != 0) ||
		    !((duty_tbl[p->duty_mode]<<p->duty_pos)&0x80))
			p->out = 0;
		else
			p->out = p->const_vol ? p->vol : p->env_vol;
	}
}


void resetapu(void)
{
	frame_counter_clock = 0;
	delayed_frame_timer_reset = 0;
	frame_counter_mode = 0;
	status = 0;
	irq_inhibit = false;
	oddtick = false;
	set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, false);
	set_irq_source(IRQ_SRC_APU_DMC_TIMER, false);

	// sound buffer, apu sample buffer
	sound_buffer_idx = 0;
	apu_samples_cnt = 0;

	// Pulse
	memset(pulse, 0, sizeof pulse);
}

void stepapu(const unsigned aputicks)
{
	for (unsigned i = 0; i < aputicks; ++i) {
		tick_frame_counter();
		tick_timer();
		if (++apu_samples_cnt == APU_SAMPLES_CNT_LIMIT) {
			apu_samples_cnt = 0;
			update_channels_output();
			const unsigned sample = pulse[0].out + pulse[1].out; 
			sound_buffer[sound_buffer_idx] = sample * 256;
			if (++sound_buffer_idx >= SOUND_BUFFER_SIZE) {
				sound_buffer_idx = 0;
				queue_sound_buffer((void*)sound_buffer,
				                   sizeof sound_buffer);
			}
		}
		oddtick = !oddtick;
	}
}


static void write_apu_status(const uint8_t val)
{
	for (unsigned i = 0; i < 2; ++i) {
		pulse[i].enabled = (val&(1<<i)) != 0;
		if (!pulse[i].enabled)
			pulse[i].len_cnt = 0;
	}
	set_irq_source(IRQ_SRC_APU_DMC_TIMER, false);
}




void apuwrite(const uint8_t val, const uint16_t addr)
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

uint8_t apuread_status(void)
{
	const bool frame_irq = get_irq_source(IRQ_SRC_APU_FRAME_COUNTER);
	const bool dmc_irq = get_irq_source(IRQ_SRC_APU_DMC_TIMER);
	set_irq_source(IRQ_SRC_APU_FRAME_COUNTER, false);

	return (dmc_irq<<7)|(frame_irq<<6)|
		((pulse[1].len_cnt > 0)<<1)|(pulse[0].len_cnt > 0);
}
