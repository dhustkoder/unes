#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "cpu.h"
#include "ppu.h"
#include "rom.h"

extern enum NTMirroringMode ppu_ntmirroring_mode;
uint8_t* ppu_patterntable_upper;
uint8_t* ppu_patterntable_lower;

static int32_t prgrom_size;
static int32_t chr_size; // chrrom or chrram size
static int32_t sram_size;
static uint8_t mappertype;
static const char* rompath;
static uint8_t* cartdata; // prgrom, chrrom or chrram, sram


// support only NROM and partially MMC1 for now
static struct {
	uint8_t match[4];
	uint8_t prgrom_nbanks;  // number of 16 kib prg-rom banks.
	uint8_t chrrom_nbanks;  // nuber of 8 kib vrom banks
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t sram_nbanks;    // number of 8 kib ram banks
	// uint8_t reserved[7];
	// uint8_t trainer[];   // not supported yet
} ines;


static union {
	struct {
		uint16_t prg_addr_mask;
	} nrom;
	struct {
		uint_fast8_t reg[4];
		uint_fast8_t tmp;
		int_fast8_t shiftcnt;
	} mmc1;
} mapper;



bool loadrom(const char* const path)
{
	bool ret = false;

	rompath = path;
	FILE* const file = fopen(rompath, "r");
	if (file == NULL) {
		fprintf(stderr, "Couldn't open file \'%s\': %s\n",
			rompath, strerror(errno));
		return false;
	}

	const uint8_t match[] = { 'N', 'E', 'S', 0x1A };
	if (fread(&ines, 1, 9, file) < 9 ||
	    memcmp(ines.match, match, sizeof match) != 0) {
		fprintf(stderr, "\'%s\' is not an ines file.\n", rompath);
		goto Lfclose;
	}
	
	// check cartridge compatibility
	mappertype = (ines.ctrl2&0xF0)|((ines.ctrl1&0xF0)>>4);
	if (mappertype > MMC1) {
		fprintf(stderr, "mapper %d not supported.\n", mappertype);
		goto Lfclose;
	} else if ((ines.ctrl1&0x08) != 0) {
		fprintf(stderr, "four screen mirroring not supported.\n");
		goto Lfclose;
	} else if ((ines.ctrl1&0x04) != 0) {
		fprintf(stderr, "trainer is not supported.\n");
		goto Lfclose;
	} else if (ines.sram_nbanks > 1) {
		fprintf(stderr, "sram bank switching not supported.\n");
		goto Lfclose;
	}

	const uint32_t chrrom_size = ines.chrrom_nbanks * CHR_BANK_SIZE;
	const uint32_t chrram_size = chrrom_size ? 0 : CHR_BANK_SIZE;
	chr_size = chrrom_size ? chrrom_size : chrram_size;
	prgrom_size = ines.prgrom_nbanks * PRGROM_BANK_SIZE;
	sram_size = ines.sram_nbanks != 0
	            ? ines.sram_nbanks * SRAM_BANK_SIZE
	            : SRAM_BANK_SIZE;

	const uint32_t read_size = prgrom_size + chrrom_size;
	cartdata = malloc(read_size + chrram_size + sram_size);
	fseek(file, 0x10, SEEK_SET);
	if (fread(cartdata, 1, read_size, file) < read_size) {
		fprintf(stderr, "Couldn't read \'%s\' properly\n", rompath);
		freerom();
		goto Lfclose;
	}

	printf("PRG-ROM BANKS: %" PRIi32 " x 16Kib = %" PRIi32 "\n"
	       "CHR-ROM BANKS: %" PRIi32 " x 8 Kib = %" PRIi32 "\n"
	       "CHR-RAM BANKS: %" PRIi32 " x 8 Kib = %" PRIi32 "\n"
	       "SRAM BANKS:    %" PRIi32 " x 8 Kib = %" PRIi32 "\n"
	       "CTRL BYTE 1:\n"
	       "\tMIRRORING: %d = %s\n"
	       "\tBATTERY-BACKED RAM AT $6000-$7FFF: %d = %s\n"
	       "\tTRAINER AT $7000-71FF: %d = %s\n"
	       "\tFOUR SCREEN MIRRORING: %d = %s\n"
	       "\tFOUR LOWER BITS OF MAPPER NUMBER: $%.1x\n"
	       "CTRL BYTE 2:\n"
	       "\tBITS 0-3 RESERVED FOR FUTURE USE AND SHOULD ALL BE 0: $%.1x\n"
	       "\tFOUR UPPER BITS OF MAPPER NUMBER: $%.1x\n"
	       "MAPPER: %" PRIu8 "\n",
	       prgrom_size / PRGROM_BANK_SIZE, prgrom_size,
	       chrrom_size / CHR_BANK_SIZE, chrrom_size,
	       chrram_size / CHR_BANK_SIZE, chrram_size,
	       sram_size / SRAM_BANK_SIZE, sram_size,
	       (ines.ctrl1&0x01), (ines.ctrl1&0x01) ? "VERTICAL" : "HORIZONTAL",
	       (ines.ctrl1&0x02)>>1, (ines.ctrl1&0x02) ? "YES" : "NO",
	       (ines.ctrl1&0x04)>>2, (ines.ctrl1&0x04) ? "YES" : "NO",
	       (ines.ctrl1&0x08)>>3, (ines.ctrl1&0x08) ? "YES" : "NO",
	       (ines.ctrl1&0xF0)>>4, ines.ctrl2&0x0F,
	       (ines.ctrl2&0xF0)>>4, mappertype);
	
	// init mapper
	memset(&mapper, 0, sizeof mapper);
	switch (mappertype) {
	case NROM:
		mapper.nrom.prg_addr_mask = ines.prgrom_nbanks > 1
			                    ? 0x7FFF
					    : 0x3FFF;
		ppu_ntmirroring_mode = (ines.ctrl1&0x01)
		                ? NTMIRRORING_VERTICAL
		                : NTMIRRORING_HORIZONTAL;
		ppu_patterntable_lower = &cartdata[prgrom_size];
		ppu_patterntable_upper = &ppu_patterntable_lower[0x1000];
		break;
	case MMC1:
		mapper.mmc1.reg[0] = 0x0C;
		ppu_patterntable_lower = &cartdata[prgrom_size];
		ppu_patterntable_upper = &ppu_patterntable_lower[0x1000];
		break;
	}

	ret = true;

Lfclose:
	fclose(file);
	return ret;
}

void freerom(void)
{
	free(cartdata);
}


// NROM
static uint_fast8_t nrom_read(const uint_fast16_t addr)
{
	return cartdata[addr&mapper.nrom.prg_addr_mask];
}

static void nrom_write(const uint_fast8_t value, const uint_fast16_t addr)
{
	((void)value);
	((void)addr);
}

// MMC1
/*
static void mmc1_eval_prgbank(void)
{
	const uint_fast8_t reg3 = mapper.mmc1.reg[3]&0x0F;
	switch ((mapper.mmc1.reg[0]&0x0C)>>2) {
	case 0x00:
		// switch 32kb at $8000
		break;
	case 0x01:
		// switch 32kb at $8000 ignoring low bit of bank number
		break;
	case 0x02:
		// fix first bank at $8000 and switch 16kb banks at $C000
		break;
	default:
		// fix last bank at $C000 and switch 16kb banks at $8000
		break;
	}
}
*/
static uint_fast8_t mmc1_read(const uint_fast16_t addr)
{
	const uint_fast8_t reg3 = mapper.mmc1.reg[3]&0x0F;

	unsigned bank;
	uint_fast8_t data;
	switch ((mapper.mmc1.reg[0]&0x0C)>>2) {
	case 0x00:
		// switch 32kb at $8000
		bank = PRGROM_BANK_SIZE * 2 * reg3;
		data = cartdata[bank + addr - ADDR_PRGROM];
		break;
	case 0x01:
		// switch 32kb at $8000 ignoring low bit of bank number
		bank = PRGROM_BANK_SIZE * 2 * (reg3&0x0E);
		data = cartdata[bank + addr - ADDR_PRGROM];
		break;
	case 0x02:
		// fix first bank at $8000 and switch 16kb banks at $C000
		if (addr < ADDR_PRGROM_UPPER) {
			data = cartdata[addr - ADDR_PRGROM];
			break;
		}
		bank = PRGROM_BANK_SIZE * reg3;
		data = cartdata[bank + addr - ADDR_PRGROM_UPPER];
		break;
	default:
		// fix last bank at $C000 and switch 16kb banks at $8000
		if (addr >= ADDR_PRGROM_UPPER) {
			bank = prgrom_size - PRGROM_BANK_SIZE;
			data = cartdata[bank + addr - ADDR_PRGROM_UPPER];
			break;
		}
		bank = PRGROM_BANK_SIZE * reg3;
		data = cartdata[bank + addr - ADDR_PRGROM];
		break;
	}

	return data;
}

static void mmc1_write(const uint_fast8_t value, const uint_fast16_t addr)
{
	if ((value&0x80) == 0) {
		mapper.mmc1.tmp = (mapper.mmc1.tmp>>1)|((value&0x01)<<4);
		if (++mapper.mmc1.shiftcnt == 5) {
			mapper.mmc1.shiftcnt = 0;
			const unsigned regidx = (addr&0x6000)>>13;
			mapper.mmc1.reg[regidx] = mapper.mmc1.tmp;
			if (regidx != 3) {
				const uint8_t modes[] = {
					NTMIRRORING_ONE_SCREEN_LOW,
					NTMIRRORING_ONE_SCREEN_UPPER,
					NTMIRRORING_VERTICAL,
					NTMIRRORING_HORIZONTAL
				};
				ppu_ntmirroring_mode = modes[mapper.mmc1.reg[0]&0x03];
				const uint8_t* const reg = mapper.mmc1.reg;
				uint8_t* const chr = &cartdata[prgrom_size];
				if ((reg[0]&0x10) == 0) { // switch 8kb banks at $0000 - $1FFF
					ppu_patterntable_lower = &chr[(reg[1]&0x1E) * 8192];
					ppu_patterntable_upper = &ppu_patterntable_lower[0x1000];
				} else {
					// switch 4kb banks at $0000 - $0FFF
					ppu_patterntable_lower = &chr[reg[1] * 4096];
			                // switch 4kb banks at $1000 - $1FFF
					ppu_patterntable_upper = &chr[reg[2] * 4096];
				}
			}
		}
	} else {
		mapper.mmc1.shiftcnt = 0;
		mapper.mmc1.reg[0] |= 0x0C;
	}
}

uint_fast8_t romread(const uint_fast16_t addr)
{
	if (addr >= ADDR_PRGROM) {
		switch (mappertype) {
		case NROM: return nrom_read(addr);       break;
		default:/*MMC1*/ return mmc1_read(addr); break;
		}
	} else {
		return cartdata[prgrom_size + chr_size + addr - ADDR_SRAM];
	}
}

void romwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	if (addr >= ADDR_PRGROM) {
		switch (mappertype) {
		case NROM: nrom_write(value, addr); break;
		case MMC1: mmc1_write(value, addr); break;
		}
	} else {
		cartdata[prgrom_size + chr_size + addr - ADDR_SRAM] = value;
	}
}

