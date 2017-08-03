#include <string.h>
#include <stdbool.h>
#include "cpu.h"
#include "ppu.h"

static uint_fast8_t openbus;
static uint_fast8_t ctrl;           // $2000
static uint_fast8_t mask;           // $2001
static uint_fast8_t status;         // $2002
static uint_fast8_t scroll;         // $2005
static uint_fast8_t spr_addr;       // $2003 
static uint_fast16_t vram_addr;     // $2006
static bool vram_addr_phase;

static int_fast16_t ppuclk;         // 0 - 341
static int_fast16_t scanline;       // 0 - 262
static bool nmi_occurred;
static bool nmi_output;
static bool odd_frame;
static bool nmi_for_frame;

static uint8_t oam[0x100];


static uint_fast8_t read_status(void)
{
	const uint_fast8_t b7 = nmi_occurred ? 1 : 0;
	nmi_occurred = false;
	return (b7<<7)|(openbus&0x1F);
}

static uint_fast8_t read_oam(void)
{
	return oam[spr_addr];
}

static uint_fast8_t read_vram_data(void)
{
	//return vram[vram_addr];
	return 0x00;
}

static void write_ctrl(const uint_fast8_t val)
{
	ctrl = val;
	nmi_output = (val&0x80) == 0x80;
}

static void write_mask(const uint_fast8_t val)
{
	mask = val;
}

static void write_vram_addr(const uint_fast8_t val)
{
	if (vram_addr_phase)
		vram_addr = (vram_addr&0xFF00)|val;
	else
		vram_addr = (vram_addr&0x00FF)|(val<<8);

	vram_addr_phase = !vram_addr_phase;
}

void resetppu(void)
{
	openbus = 0x00;
	ctrl = 0x00;
	mask = 0x00;
	status = 0xA0;
	scroll = 0x00;
	spr_addr = 0x00;
	vram_addr = 0x0000;
	vram_addr_phase = false;

	ppuclk = 0;
	scanline = 240;
	nmi_occurred = false;
	nmi_output = false;
	nmi_for_frame = false;
	odd_frame = false;
}

void stepppu(const int_fast32_t pputicks)
{
	for (int_fast32_t i = 0; i < pputicks; ++i) {
		if (++ppuclk == 341) {
			ppuclk = 0;
			if (++scanline == 262) {
				scanline = 0;
				if ((mask&0x18) == 0 && odd_frame)
					++ppuclk;
				odd_frame = !odd_frame;
			}
		}

		if (!nmi_for_frame && nmi_occurred && nmi_output) {
			trigger_nmi();
			nmi_for_frame = true;
		}

		if (scanline == 241) {
			if (ppuclk == 1) {
				nmi_occurred = true;
				nmi_for_frame = false;
			}
		} else if (scanline == 261) {
			if (ppuclk == 2)
				nmi_occurred = false;
		}
	}
}

void ppuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	switch (addr&0x0007) {
	case 0: write_ctrl(val); break;
	case 1: write_mask(val); break;
	case 3: spr_addr = val; break;
	case 6: write_vram_addr(val); break;
	}
	openbus = val;
}

uint_fast8_t ppuread(const uint_fast16_t addr)
{
	switch (addr&0x0007) {	
	case 2: return read_status();   break;
	case 4: return read_oam(); break;
	case 7: return read_vram_data(); break;
	}
	return openbus;
}

