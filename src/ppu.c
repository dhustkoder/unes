#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "video.h"
#include "cpu.h"
#include "rom.h"
#include "ppu.h"


#define SCREEN_WIDTH      (256)
#define SCREEN_HEIGHT     (240)


static uint_fast8_t ppuopenbus;
static uint_fast8_t ppuctrl;     // $2000
static uint_fast8_t ppumask;     // $2001
static uint_fast8_t ppustatus;   // $2002
static uint_fast8_t oamaddr;     // $2003 
static uint_fast16_t ppuscroll;  // $2005
static int_fast16_t ppuaddr;     // $2006
static bool ppuaddr_phase;
static bool ppuscroll_phase;

static int_fast16_t ppuclk;      // 0 - 341
static int_fast16_t scanline;    // 0 - 262
static bool nmi_occurred;
static bool nmi_output;
static bool oddframe;
static bool nmi_for_frame;

uint8_t ppu_oam[0x100];
static uint8_t nametables[0x800];
static uint8_t palettes[0x19];
static uint32_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

static const uint32_t rgb_palette[0x40] = {
	0x7C7C7C, 0x0000FC, 0x0000BC, 0x4428BC, 0x940084, 0xA80020, 0xA81000, 0x881400,
	0x503000, 0x007800, 0x006800, 0x005800, 0x004058, 0x000000, 0x000000, 0x000000,
	0xBCBCBC, 0x0078F8, 0x0058F8, 0x6844FC, 0xD800CC, 0xE40058, 0xF83800, 0xE45C10,
	0xAC7C00, 0x00B800, 0x00A800, 0x00A844, 0x008888, 0x000000, 0x000000, 0x000000,
	0xF8F8F8, 0x3CBCFC, 0x6888FC, 0x9878F8, 0xF878F8, 0xF85898, 0xF87858, 0xFCA044,
	0xF8B800, 0xB8F818, 0x58D854, 0x58F898, 0x00E8D8, 0x787878, 0x000000, 0x000000,
	0xFCFCFC, 0xA4E4FC, 0xB8B8F8, 0xD8B8F8, 0xF8B8F8, 0xF8A4C0, 0xF0D0B0, 0xFCE0A8,
	0xF8D878, 0xD8F878, 0xB8F8B8, 0xB8F8D8, 0x00FCFC, 0xF8D8F8, 0x000000, 0x000000
};


static int_fast16_t eval_nt_offset(uint_fast16_t addr)
{
	assert(addr >= 0x2000 && addr <= 0x3EFF);

	int_fast16_t offset = 0;
	switch (get_ntmirroring_mode()) {
	case NTMIRRORING_HORIZONTAL:
		offset = ((addr>>1)&0x400) + (addr&0x3FF);
		break;
	case NTMIRRORING_VERTICAL:
		offset = addr&0x7FF;
		break;
	case NTMIRRORING_ONE_SCREEN_LOW:
		offset = addr&0x3FF;
		break;
	case NTMIRRORING_ONE_SCREEN_UPPER:
		offset = 0x400 + (addr&0x3FF);
		break;
	}

	return offset;
}

static int_fast8_t eval_palette_offset(uint_fast16_t addr)
{
	assert((addr >= 0x3F00 && addr <= 0x3FFF) || addr <= 0x1F);

	if (addr >= 0x3F00)
		addr &= 0x1F;

	return (addr&0x03) ? addr - (addr>>2) : 0;
}

static void draw_bg_scanline(void)
{
	assert(scanline >= 0 && scanline <= 239); // visible lines

	static const uint16_t nt_addrs[] = {
		0x2000, 0x2400, 0x2800, 0x2C00
	};

	const uint8_t* const nt = &nametables[eval_nt_offset(nt_addrs[ppuctrl&0x03])];
	const uint8_t* const at = nt + 0x3C0;
	const uint8_t greymsk = (ppumask&0x01) ? 0x30 : 0xFF;
	const int bgpattern = (ppuctrl&0x10) ? 0x1000 : 0x0000;
	const int spritey = scanline&0x07;
	const int ysprite = scanline / 8;

	for (int i = 0; i < 32; ++i) {
		unsigned palnum = at[(((ysprite / 4) * 8) + (i / 4))&0x3F];
		palnum >>= ((i&0x03) > 1) ? 2 : 0;
		palnum >>= ((ysprite&0x03) > 1) ? 4 : 0;
		palnum &= 0x03;

		const int spridx = nt[ysprite * 32 + i] * 16;
		uint8_t b0 = romchrread(bgpattern + spridx + spritey);
		uint8_t b1 = romchrread(bgpattern + spridx + spritey + 8);
		for (int p = 0; p < 8; ++p) {
			const int c = ((b1>>6)&0x02)|(b0>>7);
			const int pal = palettes[eval_palette_offset(palnum * 4 + c)];
			const uint32_t color = rgb_palette[pal&greymsk&0x3F];
			screen[scanline][i * 8 + p] = color;
			b0 <<= 1;
			b1 <<= 1;
		}
	}

	oamaddr = 0;
}

static void draw_sprite_scanline(void)
{
	static const struct {
		uint8_t y;
		uint8_t tile;
		uint8_t attr;
		uint8_t x;
	} *pspr;

	for (int i = 0; i < 0x100; i += 4) {
		pspr = (void*)&ppu_oam[i];
		if (pspr->y > scanline || (scanline >= (pspr->y + 8)))
			continue;

		const int spry = (pspr->attr&0x80) != 0
		 ? -((scanline - pspr->y) - 7) // flip vertically
		 : (scanline - pspr->y);

		const int sprx = pspr->x;
		const int pattern = (ppuctrl&0x08) ? 0x1000 : 0x0000;
		const int tileidx = pspr->tile * 16;
		const int palidx = 0x10 + (pspr->attr&0x03) * 4;
		uint8_t b0 = romchrread(pattern + tileidx + spry);
		uint8_t b1 = romchrread(pattern + tileidx + spry + 8);

		if ((pspr->attr&0x40) != 0) {
			// flip horizontally
			uint8_t tmp0 = 0;
			uint8_t tmp1 = 0;
			for (int j = 0; j < 8; ++j) {
				tmp0 |= ((b0>>j)&0x01)<<(7 - j);
				tmp1 |= ((b1>>j)&0x01)<<(7 - j);
			}
			b0 = tmp0;
			b1 = tmp1;
		}

		for (int p = 0; p < 8 && (sprx + p) < 256; ++p) {
			const int c = ((b1>>6)&0x02)|(b0>>7);
			b0 <<= 1;
			b1 <<= 1;

			if (c == 0)
				continue;

			const int pal = palettes[eval_palette_offset(palidx + c)];
			const uint32_t color = rgb_palette[pal&0x3F];
			screen[scanline][sprx + p] = color;
		}
	}
}


void resetppu(void)
{
	ppuopenbus = 0x00;
	ppuctrl = 0x00;
	ppumask = 0x00;
	ppustatus = 0xA0;
	oamaddr = 0x00;
	ppuscroll = 0x0000;
	ppuaddr = 0x0000;
	ppuaddr_phase = false;
	ppuscroll_phase = false;

	ppuclk = 0;
	scanline = 240;
	nmi_occurred = false;
	nmi_output = false;
	nmi_for_frame = false;
	oddframe = false;
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
			if (scanline < 240) {
				if ((ppumask&0x08) != 0)
					draw_bg_scanline();
				if ((ppumask&0x10) != 0)
					draw_sprite_scanline();
			}
			if (scanline++ == 261) {
				scanline = 0;
				render((const uint32_t*)screen, sizeof(screen));
				if ((ppumask&0x18) && oddframe)
					++ppuclk;
				oddframe = !oddframe;
			}
		}
	}
}


// Registers read/write for CPU
static uint_fast8_t read_ppustatus(void)
{
	const uint_fast8_t b7 = nmi_occurred<<7;
	nmi_occurred = false;
	return b7|(ppuopenbus&0x1F);
}

static void write_ppuctrl(const uint_fast8_t val)
{
	ppuctrl = val;
	nmi_output = (val&0x80) == 0x80;
}

static void write_ppumask(const uint_fast8_t val)
{
	ppumask = val;
}


static uint_fast8_t read_oamdata(void)
{
	return ppu_oam[oamaddr];
}

static void write_oamdata(const uint_fast8_t data)
{
	ppu_oam[oamaddr++] = data;
	oamaddr &= 0xFF;
}


static void ppuaddr_inc(void)
{
	ppuaddr += (ppuctrl&0x04) == 0 ? 1 : 32;
	ppuaddr &= 0x3FFF;
}

static uint_fast8_t read_ppudata(void)
{
	uint_fast8_t r = 0;
	if (ppuaddr < 0x2000)
		r = romchrread(ppuaddr);
	else if (ppuaddr < 0x3F00)
		r = nametables[eval_nt_offset(ppuaddr)];
	else
		r = palettes[eval_palette_offset(ppuaddr)];

	ppuaddr_inc();
	return r;
}

static void write_ppudata(const uint_fast8_t val)
{
	if (ppuaddr < 0x2000)
		romchrwrite(val, ppuaddr);
	else if (ppuaddr < 0x3F00)
		nametables[eval_nt_offset(ppuaddr)] = val;
	else 
		palettes[eval_palette_offset(ppuaddr)] = val;

	ppuaddr_inc();
}

static void write_ppuaddr(const uint_fast8_t val)
{
	if (ppuaddr_phase) {
		ppuaddr |= val;
	} else {
		ppuaddr = 0;
		ppuaddr = val<<8;
	}

	ppuaddr &= 0x3FFF;
	ppuaddr_phase = !ppuaddr_phase;
}

static void write_ppuscroll(const uint_fast8_t val)
{
	if (ppuscroll_phase) {
		ppuscroll |= val;
	} else {
		ppuscroll = 0;
		ppuscroll = val<<8;
	}

	ppuscroll_phase = !ppuscroll_phase;
}


void ppuwrite(const uint_fast8_t val, const uint_fast16_t addr)
{
	switch (addr&0x0007) {
	case 0: write_ppuctrl(val);   break;
	case 1: write_ppumask(val);   break;
	case 3: oamaddr = val;        break;
	case 4: write_oamdata(val);   break;
	case 5: write_ppuscroll(val); break;
	case 6: write_ppuaddr(val);   break;
	case 7: write_ppudata(val);   break;
	}
	ppuopenbus = val;
}

uint_fast8_t ppuread(const uint_fast16_t addr)
{
	switch (addr&0x0007) {	
	case 2: return read_ppustatus(); break;
	case 4: return read_oamdata();   break;
	case 7: return read_ppudata();   break;
	}
	return ppuopenbus;
}

