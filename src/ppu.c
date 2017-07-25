#include <string.h>
#include <stdbool.h>
#include "cpu.h"
#include "ppu.h"


static uint_fast8_t openbus;
static uint_fast8_t ctrl;       // $2000
static uint_fast8_t mask;       // $2001
static uint_fast8_t status;     // $2002
static uint_fast8_t scroll;     // $2005
static uint_fast8_t spr_addr;   // $2003 
static uint_fast16_t vram_addr; // $2006
static bool vram_addr_phase;

static int_fast32_t cpuclk_last;
static int_fast16_t ppuclk;     // 0 - 341
static int_fast16_t scanline;   // 0 - 262
static bool nmi_occurred, nmi_output;
static uint_fast8_t frame_toggle;

static uint8_t spr_data[0x100];
static uint8_t vram[0x4000];


static uint_fast8_t read_status(void)
{
	const uint_fast8_t b7 = nmi_occurred ? 1 : 0;
	nmi_occurred = false;
	return (b7<<7);
}

static uint_fast8_t read_spr_data(void)
{
	return spr_data[spr_addr];
}

static uint_fast8_t read_vram_data(void)
{
	return vram[vram_addr];
}

static void write_ctrl(const uint_fast8_t val)
{
	ctrl = val;
	nmi_output = (val&0x80) == 0x80;
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

	cpuclk_last = 0;
	ppuclk = 340;
	scanline = 240;
	nmi_occurred = false;
	nmi_output = false;
	frame_toggle = 0;
}

void stepppu(void)
{
	extern const int_fast32_t cpuclk;
	const int_fast32_t ticks = (cpuclk > cpuclk_last ? cpuclk - cpuclk_last : cpuclk) * 3;
	cpuclk_last = cpuclk;

	for (int_fast32_t i = 0; i < ticks; ++i) {
		if (++ppuclk == 341) {
			ppuclk = 0;
			++scanline;
			if (scanline == 241) {
				nmi_occurred = true;
				if (nmi_output)
					trigger_nmi();
			} else if (scanline == 261) {
				scanline = 0;
				nmi_occurred = false;
			}
		}
	}
}

void ppuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	switch (addr) {
	case 0x2000: write_ctrl(val); break;
	case 0x2003: spr_addr = val; break;
	case 0x2006: write_vram_addr(val); break;
	}
	openbus = val;
}

uint_fast8_t ppuread(const uint_fast16_t addr)
{
	switch (addr) {	
	case 0x2002: openbus = read_status();   break;
	case 0x2004: openbus = read_spr_data(); break;
	case 0x2007: openbus = read_vram_data(); break;
	}

	return openbus;
}

void ppu_load_chr_rom(const uint8_t* const chr)
{
	memcpy(vram, chr, 0x2000);
}

