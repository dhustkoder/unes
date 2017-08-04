#include <string.h>
#include <stdbool.h>
#include "video.h"
#include "cpu.h"
#include "rom.h"
#include "ppu.h"


#define ADDR_PATTERN_TBL1 (0x0000)
#define ADDR_PATTERN_TBL2 (0x1000)
#define ADDR_NAME_TBL1    (0x2000)
#define ADDR_NAME_TBL2    (0x2400)
#define SCREEN_WIDTH      (256)
#define SCREEN_HEIGHT     (240)


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

static const uint32_t rgb_palette[] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};

static uint8_t oam[0x100];
static uint8_t nametable[0x800];
static uint8_t palette[0x20];
static uint32_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

static void draw_sprite(const uint8_t sprite[16], const int y, const int x)
{
	static const uint32_t testpalette[] = {
		0x00000000, 0xFFFFFFFF, 0x50505050, 0x90909090
	};

	for (int i = 0; i < 8; ++i) {
		uint8_t b0 = sprite[i];
		uint8_t b1 = sprite[i + 8];
		for (int j = 0; j < 8; ++j) {
			const uint8_t c =  ((b1>>6)&0x02)|(b0>>7); 
			screen[y + i][x + j] = testpalette[c];
			b0 <<= 1;
			b1 <<= 1;
		}
	}
}

static void render_pattern_tbls(void)
{
	uint8_t sprite[16];
	for (int i = 0; i < 512; ++i) {
		for (int j = 0; j < 16; ++j)
			sprite[j] = romchrread(j + (i * 16));
		draw_sprite(sprite, (i / 32) * 8, (i * 8) & 0xFF);
	}

	render((uint32_t*)screen, sizeof(screen));
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
			if (ppuclk == 1)
				nmi_occurred = false;
		}

		if (ppuclk++ == 340) {
			ppuclk = 0;
			if (scanline++ == 261) {
				render_pattern_tbls();
				scanline = 0;
				if ((mask&0x18) && odd_frame)
					++ppuclk;
				odd_frame = !odd_frame;
			}
		}
	}
}


// Registers read/write for CPU
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
	printf("READ VRAM %lx\n", vram_addr);
	uint_fast8_t r = 0;
	if (vram_addr < 0x2000) {
		r = romchrread(vram_addr);
	} else if (vram_addr < 0x3000) {
		r = nametable[(vram_addr - 0x2000)&0x7FF];
	} else if (vram_addr < 0x3F00) {
		r = nametable[(vram_addr - 0x3000)&0x7FF];
	} else if (vram_addr < 0x3F20) {
		r = palette[vram_addr - 0x3F00];
	}

	vram_addr += (ctrl&0x04) == 0 ? 1 : 32;
	vram_addr &= 0x3FFF;
	return r;
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

static void write_vram_data(const uint_fast8_t val)
{
	printf("WRITE TO VRAM %lx: %x\n", vram_addr, val);
	if (vram_addr < 0x2000) {
		romchrwrite(val, vram_addr);
	} else if (vram_addr < 0x3000) {
		nametable[(vram_addr - 0x2000)&0x7FF] = val;
	} else if (vram_addr < 0x3F00) {
		nametable[(vram_addr - 0x3000)&0x7FF] = val;
	} else if (vram_addr < 0x3F20) {
		palette[vram_addr - 0x3F00] = val;
	}

	vram_addr += (ctrl&0x04) == 0 ? 1 : 32;
	vram_addr &= 0x3FFF;
}

static void write_vram_addr(const uint_fast8_t val)
{
	if (vram_addr_phase) {
		vram_addr |= val;
	} else {
		vram_addr = 0;
		vram_addr = val<<8;
	}

	vram_addr &= 0xFFFF;
	vram_addr_phase = !vram_addr_phase;
}


void ppuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	if (addr == 0x4014) {
		// TODO OAM DMA TRANSFER
		notify_oam_dma();
		return;
	}

	switch (addr&0x0007) {
	case 0: write_ctrl(val);      break;
	case 1: write_mask(val);      break;
	case 3: spr_addr = val;       break;
	case 6: write_vram_addr(val); break;
	case 7: write_vram_data(val); break;
	}
	openbus = val;
}

uint_fast8_t ppuread(const uint_fast16_t addr)
{
	switch (addr&0x0007) {	
	case 2: return read_status();    break;
	case 4: return read_oam();       break;
	case 7: return read_vram_data(); break;
	}
	return openbus;
}

