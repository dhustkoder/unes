#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>
#include "video.h"
#include "cpu.h"
#include "rom.h"
#include "ppu.h"


#define PPU_FRAME_TICKS (341)

// cpu.c
extern bool cpu_nmi;


// ppu.c globals
nt_mirroring_mode_t ppu_ntmirroring_mode;
const void* ppu_pattern_base_addr;
uint8_t* ppu_pattern[2]; // lower and upper tables
bool ppu_need_screen_update;
uint8_t ppu_oam[0x100];

// ppu.c
static uint8_t ppuopenbus;
static uint8_t ppuctrl;     // $2000
static uint8_t ppumask;     // $2001
static uint8_t ppustatus;   // $2002
static uint8_t oamaddr;     // $2003 
static uint16_t ppuscroll;  // $2005
static int16_t ppuaddr;     // $2006

static long ppuclk;         // 0 - 341
static int16_t scanline;    // 0 - 262

static struct {
	bool scanline_drawn  : 1;
	bool nmi_occurred    : 1;
	bool spr_overflow    : 1;
	bool spr0_hit        : 1;
	bool nmi_output      : 1;
	bool oddframe        : 1;
	bool nmi_for_frame   : 1;
	bool write_toggle    : 1;
	bool need_render     : 1;
} states;

static uint8_t nametables[0x800];
static uint8_t palettes[0x1C];
static uint8_t framebuffer[NES_SCR_HEIGHT][NES_SCR_WIDTH];


static int16_t eval_nt_offset(const uint16_t addr)
{
	switch (ppu_ntmirroring_mode) {
	case NT_MIRRORING_MODE_HORIZONTAL:
		return ((addr>>1)&0x400) + (addr&0x3FF);
	case NT_MIRRORING_MODE_VERTICAL:
		return addr&0x7FF;
	case NT_MIRRORING_MODE_ONE_SCREEN_LOW:
		return addr&0x3FF;
	default:/*NT_MIRRORING_MODE_ONE_SCREEN_UPPER*/
		return 0x400 + (addr&0x3FF);
	}
}

static uint8_t eval_pal_rw_offset(const uint16_t addr)
{
	static const uint8_t mirrors[0x20] = {
		0x00, 0x01, 0x02, 0x03,
		0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B,
		0x0C, 0x0D, 0x0E, 0x0F,
		0x00, 0x10, 0x11, 0x12,
		0x04, 0x13, 0x14, 0x15,
		0x08, 0x16, 0x17, 0x18,
		0x0C, 0x19, 0x1A, 0x1B
	};

	return mirrors[addr&0x001F];
}

static uint8_t get_palette(const uint16_t addr)
{
	static const uint8_t mirrors[0x20] = {
		0x00, 0x01, 0x02, 0x03,
		0x00, 0x05, 0x06, 0x07,
		0x00, 0x09, 0x0A, 0x0B,
		0x00, 0x0D, 0x0E, 0x0F,
		0x00, 0x10, 0x11, 0x12,
		0x00, 0x13, 0x14, 0x15,
		0x00, 0x16, 0x17, 0x18,
		0x00, 0x19, 0x1A, 0x1B
	};

	return palettes[mirrors[addr&0x001F]]&0x3F;
}

static void draw_bg_scanline(void)
{
	const unsigned xscroll = (ppuscroll>>8)&0xFF;
	const unsigned xscroll_div = xscroll>>3;
	const unsigned xscroll_mod = xscroll&0x07;

	const uint8_t* const pattern = ppu_pattern[(ppuctrl&0x10)>>4];
	const unsigned greymsk = (ppumask&0x01) ? 0x30 : 0xFF;
	const unsigned spritey = scanline&0x07;
	const unsigned ysprite = scanline>>3;
	const unsigned palrowshift = (ysprite&0x03) > 1 ? 4 : 0;

	const uint8_t* nt = &nametables[eval_nt_offset(((ppuctrl&0x03)<<10))];
	const uint8_t* at = nt + 0x3C0;
	bool changed = false;

	uint8_t* pixels = &framebuffer[scanline][0];

	const unsigned ntiles = xscroll_mod ? 33 : 32;

	for (unsigned i = 0, nt_tile_idx = xscroll_div; i < ntiles; ++i, ++nt_tile_idx) {
		if (!changed && (xscroll_div + i) >= 32) {
			nt = &nametables[eval_nt_offset(((ppuctrl&0x03)<<10) + 0x400)];
			at = nt + 0x3C0;
			nt_tile_idx = 0;
			changed = true;
		}

		unsigned palrow = at[((ysprite>>2)<<3) + (nt_tile_idx>>2)];
		palrow >>= ((nt_tile_idx&0x03) > 1) ? 2 : 0;
		palrow >>= palrowshift;
		palrow &= 0x03;
		palrow <<= 2;

		const unsigned spridx = nt[((ysprite<<5) + nt_tile_idx)]<<4;
		const unsigned pbeg = (i == 0) ?  xscroll_mod : 0;
		const unsigned pend = (i == 32) ? xscroll_mod : 8;
		unsigned b0 = pattern[spridx + spritey]<<pbeg;
		unsigned b1 = pattern[spridx + spritey + 8]<<pbeg;

		for (unsigned p = pbeg; p < pend; ++p) {
			const unsigned c = ((b1>>6)&0x02)|((b0>>7)&0x01);
			const unsigned paladdr = palrow|c;
			const unsigned pal = get_palette((uint16_t)paladdr);
			*pixels = (uint8_t)(pal&greymsk);

			/* using the 8th bit to flag if bkg pixel is transparent */
			if (c == 0)
				*pixels |= 0x80;

			++pixels;
			b0 <<= 1;
			b1 <<= 1;
		}
	}

	assert((pixels - (&framebuffer[scanline][0])) == NES_SCR_WIDTH);
}

static void draw_sprite_scanline(void)
{
	const unsigned ypos = scanline;
	const unsigned sprh = (ppuctrl&0x20) ? 16 : 8;
	struct {
		uint8_t y;
		uint8_t tile;
		uint8_t attr;
		uint8_t x;
	} *spr = (void*) (ppu_oam + 0xFC);
	for (unsigned drawn = 0; (uint8_t*)spr >= ppu_oam; --spr) {
		if ((spr->y + 1u) == 0 || (spr->y + 1u) > ypos ||
		    (ypos >= ((spr->y + 1u) + sprh)))
			continue;

		if (!states.spr_overflow && drawn >= 8) {
			states.spr_overflow = true;
		}

		++drawn;
		const unsigned tiley = (spr->attr&0x80) == 0
		                 ? (ypos - (spr->y + 1u))
		                 : (unsigned) -(signed)((ypos - (spr->y + 1u)) - (sprh - 1u)); // flip vertically
		unsigned tileidx;
		const uint8_t* pattern;
		if (sprh == 8) {
			tileidx = spr->tile<<4;
			pattern = ppu_pattern[(ppuctrl&0x08)>>3];
		} else {
			tileidx = (spr->tile>>1)<<5;
			pattern = ppu_pattern[spr->tile&0x01];
			if (tiley > 7)
				tileidx += 8;
		}

		uint8_t b0 = pattern[tileidx + tiley];
		uint8_t b1 = pattern[tileidx + tiley + 8];
		if ((spr->attr&0x40) != 0) {
			// flip horizontally
			uint8_t tmp0 = 0;
			uint8_t tmp1 = 0;
			for (uint8_t j = 0; j < 8; ++j) {
				tmp0 |= ((b0>>j)&0x01)<<(7 - j);
				tmp1 |= ((b1>>j)&0x01)<<(7 - j);
			}
			b0 = tmp0;
			b1 = tmp1;
		}

		for (unsigned p = 0; p < 8; ++p) {
			const uint16_t c = ((b1>>6)&0x02)|(b0>>7);
			b0 <<= 1;
			b1 <<= 1;
			uint8_t* const pix = &framebuffer[ypos][(spr->x + p)&0xFF];
			if (c != 0 && !(*pix&0x80)) {
				if (!states.spr0_hit && ((uint8_t*)spr) == ppu_oam)
					states.spr0_hit = true;
				if (spr->attr&0x20)
					continue;
			} else if (c == 0) {
				continue;
			}

			const uint16_t paladdr = 0x10|((spr->attr&0x03)<<2)|c;
		 	*pix = get_palette(paladdr);
		}
	}

	oamaddr = 0;
}

static uint8_t read_ppustatus(void)
{
	const uint8_t b7 = states.nmi_occurred<<7;
	const uint8_t b6 = states.spr0_hit<<6;
	const uint8_t b5 = states.spr_overflow<<5;
	states.nmi_occurred = false;
	return b7|b6|b5|(ppuopenbus&0x1F);
}

static void write_ppuctrl(const uint8_t val)
{
	ppuctrl = val;
	states.nmi_output = (val&0x80) == 0x80;
}

static void write_ppumask(const uint8_t val)
{
	ppumask = val;
}


static uint8_t read_oamdata(void)
{
	return ppu_oam[oamaddr];
}

static void write_oamdata(const uint8_t data)
{
	if (ppu_oam[oamaddr] != data) {
	       ppu_oam[oamaddr]	= data;
	       ppu_need_screen_update = true;
	}
	++oamaddr;
	oamaddr &= 0xFF;
}


static void ppuaddr_inc(void)
{
	ppuaddr += (ppuctrl&0x04) == 0 ? 1 : 32;
	ppuaddr &= 0x3FFF;
}

static uint8_t read_ppudata(void)
{
	uint8_t r;
	if (ppuaddr < 0x2000)
		r = ppu_pattern[(ppuaddr>>12)&0x01][ppuaddr&0xFFF];
	else if (ppuaddr < 0x3F00)
		r = nametables[eval_nt_offset(ppuaddr)];
	else
		r = palettes[eval_pal_rw_offset(ppuaddr)];

	ppuaddr_inc();
	return r;
}

static void write_ppudata(const uint8_t val)
{
	extern bool rom_chrdata_is_ram;

	uint8_t* dest;
	if (ppuaddr < 0x2000) {
		if (!rom_chrdata_is_ram)
			goto Lppuaddr_inc;
		dest = &ppu_pattern[(ppuaddr>>12)&0x01][ppuaddr&0xFFF];
	} else if (ppuaddr < 0x3F00) {
		dest = &nametables[eval_nt_offset(ppuaddr)];
	} else {
		dest = &palettes[eval_pal_rw_offset(ppuaddr)];
	}

	if (*dest != val) {
		*dest = val;
		ppu_need_screen_update = true;
	}

Lppuaddr_inc:
	ppuaddr_inc();
}

static void write_ppuaddr(const uint8_t val)
{
	if (states.write_toggle) {
		ppuaddr = (ppuaddr&0xFF00)|val;
	} else {
		ppuaddr = (ppuaddr&0x00FF)|(val<<8);
	}

	ppuaddr &= 0x3FFF;
	states.write_toggle = !states.write_toggle;
}

static void write_ppuscroll(const uint8_t val)
{
	if (states.write_toggle)
		ppuscroll |= val;
	else
		ppuscroll = val<<8;

	states.write_toggle = !states.write_toggle;
}




void ppu_reset(void)
{
	ppuopenbus = 0x00;
	ppuctrl = 0x00;
	ppumask = 0x00;
	ppustatus = 0xA0;
	oamaddr = 0x00;
	ppuscroll = 0x0000;
	ppuaddr = 0x0000;
	ppuclk = 0;
	scanline = 240;
	ppu_need_screen_update = false;
	memset(&states, 0, sizeof states);
	memset(framebuffer, 0x0D, sizeof framebuffer);
}

void ppu_step(const unsigned pputicks)
{
	ppuclk -= pputicks;

	if (ppu_need_screen_update && scanline < 240 &&
	    ppuclk <= (PPU_FRAME_TICKS - 256) && !states.scanline_drawn) {
		states.scanline_drawn = true;
		if ((ppumask&0x08) != 0) {
			draw_bg_scanline();
			states.need_render = true;
		}
		if ((ppumask&0x10) != 0) {
			draw_sprite_scanline();
			states.need_render = true;
		}
	} else if (ppuclk <= 0) {
		ppuclk += PPU_FRAME_TICKS;
		states.scanline_drawn = false;
		++scanline;
		if (scanline == 262) {
			scanline = 0;
			if (states.need_render) {
				render((void*)framebuffer);
				states.need_render = false;
			}
			if ((ppumask&0x18) && states.oddframe)
				++ppuclk;
			states.oddframe = !states.oddframe;
		} else if (scanline == 241) {
			states.nmi_occurred = true;
			states.nmi_for_frame = false;
		} else if (scanline == 261) {
			states.nmi_occurred = false;
			states.spr_overflow = false;
			states.spr0_hit = false;
		}
	}

	if (!states.nmi_for_frame && states.nmi_occurred && states.nmi_output) {
		states.nmi_for_frame = true;
		ppu_need_screen_update = false;
		cpu_nmi = true;
	}
}

void ppu_log_state(void)
{
	log_info("PPU STATE: {\n"
	        "\tNT_MIRRORING_MODE: %d\n"
	        "\tPPU_PATTERN[0]: 0x%lx\n"
	        "\tPPU_PATTERN[1]: 0x%lx\n"
	        "\tPPU_NEED_SCREEN_UPDATE: %d\n"
	        "\tOPENBUS: %d\n"
	        "\tCTRL: %d\n"
	        "\tMASK: %d\n"
	        "\tSTATUS: %d\n" 
	        "\tOAM ADDR: %d\n"
	        "\tSCROLL: %" PRIu16 "\n"
	        "\tADDR: %" PRIi16 "\n"
	        "\tCLK: %li\n"
	        "\tSCANLINE: %" PRIi16 "\n"
	        "\tSTATES {\n"
	        "\t\tscanline_drawn: %d\n" 
	        "\t\tnmi_occurred: %d\n"
	        "\t\tnmi_output: %d\n" 
	        "\t\toddframe: %d\n"
	        "\t\tnmi_for_frame: %d\n"
	        "\t\twrite_toggle: %d\n"
	        "\t\tneed_render: %d\n"
	        "\t}\n"
	        "}",
	        (int)ppu_ntmirroring_mode, ppu_pattern[0] - (uint8_t*)ppu_pattern_base_addr,
	        ppu_pattern[1] - (uint8_t*)ppu_pattern_base_addr, ppu_need_screen_update,
	        ppuopenbus, ppuctrl, ppumask, ppustatus,
	        oamaddr, ppuscroll, ppuaddr, ppuclk,
	        scanline, states.scanline_drawn, states.nmi_occurred,
	        states.nmi_output, states.oddframe, states.nmi_for_frame,
	        states.write_toggle, states.need_render);	
}

void ppu_write(const uint8_t val, const uint16_t addr)
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

uint8_t ppu_read(const uint16_t addr)
{
	switch (addr&0x0007) {	
	case 2: return read_ppustatus(); break;
	case 4: return read_oamdata();   break;
	case 7: return read_ppudata();   break;
	default: return ppuopenbus;      break;
	}
}

