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
static int_fast16_t ppuclk;     // 0 - 340
static int_fast16_t scanline;   // 0 - 261


static uint8_t spr_data[0x100];
static uint8_t vram[0x4000];


static uint_fast8_t read_status(void)
{
	return (status&0xE0) | (openbus&0x1F);
}

static uint_fast8_t read_spr_data(void)
{
	return spr_data[spr_addr];
}

static uint_fast8_t read_vram_data(void)
{
	return vram[vram_addr];
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
	ctrl = 0x00;
	mask = 0x00;
	status = 0xA0;
	spr_addr = 0x00;
	scroll = 0x00;
	openbus = 0x00;
	vram_addr = 0x0000;
	vram_addr_phase = false;

	cpuclk_last = 0;
	ppuclk = 340;
	scanline = 240;
}

void stepppu(void)
{
	extern const int_fast32_t cpuclk;
	const int_fast32_t ticks = (cpuclk >= cpuclk_last ? cpuclk - cpuclk_last : cpuclk) * 3;
	cpuclk_last = cpuclk;

	for (int_fast32_t i = 0; i < ticks; ++i) {
		++ppuclk;
		if (ppuclk > 340) {
			ppuclk = 0;
			++scanline;
			if (scanline == 240 && (ctrl&0x80)) {
				trigger_nmi();
			} else if (scanline == 261) {
				scanline = 0;
			}
		}
	}
}

void ppuwrite(uint_fast8_t val, int_fast32_t addr)
{
	switch (addr) {
	case 0x2003: spr_addr = val; break;
	case 0x2006: write_vram_addr(val); break;
	}
	openbus = val;
}

uint_fast8_t ppuread(int_fast32_t addr)
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

