#include <string.h>
#include <stdbool.h>
#include "ppu.h"


int_fast32_t ppuclk;
static uint_fast8_t openbus;
static uint_fast8_t ctrl;       // $2000
static uint_fast8_t mask;       // $2001
static uint_fast8_t status;     // $2002
static uint_fast8_t scroll;     // $2005
static uint_fast8_t spr_addr;   // $2003 
static uint_fast16_t vram_addr; // $2006
static bool vram_addr_phase;
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
	ppuclk = 0;
	ctrl = 0;
	mask = 0;
	status = 0xA0;
	spr_addr = 0;
	scroll = 0x00;
	vram_addr = 0x00;
	vram_addr_phase = false;
}

void stepppu(void)
{

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

