#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "log.h"
#include "cpu.h"
#include "ppu.h"
#include "rom.h"


// ppu.c controls
extern enum NTMirroringMode ppu_ntmirroring_mode;
extern uint8_t* ppu_patterntable_upper;
extern uint8_t* ppu_patterntable_lower;
extern bool ppu_need_screen_update;
// cpu.c controls
extern uint8_t cpu_prgrom[0x8000];
extern uint8_t cpu_sram[0x2000];

// rom.c
static int32_t prgrom_size;
static int32_t chr_size;   // chrrom or chrram size
static int32_t sram_size;
static uint8_t mappertype;
static uint8_t* cartdata;  // prgrom, chrrom or chrram

// only NROM and MMC1 are supported for now
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
		uint8_t reg[4];
		uint8_t reglast[4];
		uint8_t tmp;
		int8_t shiftcnt;
	} mmc1;
} mapper;



// MMC1
static void mmc1_update(const unsigned modified_reg_index)
{
	if (modified_reg_index != 3) {
		// nametable mirroring mode
		const uint8_t modes[] = {
			NTMIRRORING_ONE_SCREEN_LOW,
			NTMIRRORING_ONE_SCREEN_UPPER,
			NTMIRRORING_VERTICAL,
			NTMIRRORING_HORIZONTAL
		};
		ppu_ntmirroring_mode = modes[mapper.mmc1.reg[0]&0x03];

		// CHR bank
		const uint8_t* const reg = mapper.mmc1.reg;
		uint8_t* const chr = &cartdata[prgrom_size];
		if ((reg[0]&0x10) == 0) {
			// switch 8kb banks at $0000 - $1FFF
			ppu_patterntable_lower = &chr[(reg[1]&0x1E) * 8192];
			ppu_patterntable_upper = ppu_patterntable_lower + 0x1000;
		} else {
			// switch 4kb banks at $0000 - $0FFF
			ppu_patterntable_lower = &chr[reg[1] * 4096];
			// switch 4kb banks at $1000 - $1FFF
			ppu_patterntable_upper = &chr[reg[2] * 4096];
		}

		ppu_need_screen_update = true;
	}

	// PRG bank
	const uint8_t reg3 = mapper.mmc1.reg[3]&0x0F;
	unsigned bank;
	switch ((mapper.mmc1.reg[0]&0x0C)>>2) {
	case 0x00:
		// switch 32kb at $8000
		bank = PRGROM_BANK_SIZE * 2 * reg3;
		memcpy(cpu_prgrom, &cartdata[bank], 0x8000);
		break;
	case 0x01:
		// switch 32kb at $8000 ignoring low bit of bank number
		bank = PRGROM_BANK_SIZE * 2 * (reg3&0x0E);
		memcpy(cpu_prgrom, &cartdata[bank], 0x8000);
		break;
	case 0x02:
		// fix first bank at $8000 and switch 16kb banks at $C000
		bank = PRGROM_BANK_SIZE * reg3;
		memcpy(cpu_prgrom, cartdata, 0x4000);
		memcpy(&cpu_prgrom[0x4000], &cartdata[bank], 0x4000);
		break;
	default:
		// fix last bank at $C000 and switch 16kb banks at $8000
		bank = prgrom_size - PRGROM_BANK_SIZE;
		memcpy(&cpu_prgrom[0x4000], &cartdata[bank], 0x4000);
		bank = PRGROM_BANK_SIZE * reg3;
		memcpy(cpu_prgrom, &cartdata[bank], 0x4000);
		break;
	}
}

static void mmc1_write(const uint8_t value, const uint16_t addr)
{
	if ((value&0x80) == 0) {
		mapper.mmc1.tmp = (mapper.mmc1.tmp>>1)|((value&0x01)<<4);
		if (++mapper.mmc1.shiftcnt == 5) {
			mapper.mmc1.shiftcnt = 0;
			const unsigned regidx = (addr&0x6000)>>13;
			mapper.mmc1.reg[regidx] = mapper.mmc1.tmp;
			if (mapper.mmc1.reglast[regidx] != mapper.mmc1.reg[regidx]) {
				mmc1_update(regidx);
				mapper.mmc1.reglast[regidx] = mapper.mmc1.reg[regidx];
			}
		}
	} else {
		mapper.mmc1.shiftcnt = 0;
		mapper.mmc1.reg[0] |= 0x0C;
		if (mapper.mmc1.reg[0] != mapper.mmc1.reglast[0]) {
			mmc1_update(0);
			mapper.mmc1.reglast[0] = mapper.mmc1.reg[0];
		}
	}
}

static void initmapper(void)
{
	memset(&mapper, 0, sizeof mapper);
	switch (mappertype) {
	case NROM:
		// cpu prg map
		if (ines.prgrom_nbanks > 1) {
			memcpy(cpu_prgrom, cartdata, 0x8000);
		} else {
			memcpy(cpu_prgrom, cartdata, 0x4000);
			memcpy(&cpu_prgrom[0x4000], cartdata, 0x4000);
		}
		// ppu map
		ppu_ntmirroring_mode = (ines.ctrl1&0x01)
			? NTMIRRORING_VERTICAL
			: NTMIRRORING_HORIZONTAL;
		ppu_patterntable_lower = &cartdata[prgrom_size];
		ppu_patterntable_upper = ppu_patterntable_lower + 0x1000;
		break;
	case MMC1:
		mapper.mmc1.reg[0] = 0x0C;
		memset(mapper.mmc1.reglast, 0xFF, sizeof mapper.mmc1.reglast);
		mmc1_update(0);
		break;
	}
}


void romwrite(const uint8_t value, const uint16_t addr)
{
	switch (mappertype) {
	case NROM: break;
	case MMC1: mmc1_write(value, addr); break;
	}
}

bool loadrom(const uint8_t* restrict const data)
{
	const uint8_t match[] = { 'N', 'E', 'S', 0x1A };
	memcpy(&ines, data, sizeof ines);
	if (memcmp(ines.match, match, sizeof match) != 0) {
		logerror("file is not an ines file.\n");
		return false;
	}
	
	// check cartridge compatibility
	mappertype = (ines.ctrl2&0xF0)|((ines.ctrl1&0xF0)>>4);
	if (mappertype > MMC1) {
		logerror("mapper %d not supported.\n", mappertype);
		return false;
	} else if ((ines.ctrl1&0x08) != 0) {
		logerror("four screen mirroring not supported.\n");
		return false;
	} else if ((ines.ctrl1&0x04) != 0) {
		logerror("trainer is not supported.\n");
		return false;
	} else if (ines.sram_nbanks > 1) {
		logerror("sram bank switching not supported.\n");
		return false;
	}

	const uint32_t chrrom_size = ines.chrrom_nbanks * CHR_BANK_SIZE;
	const uint32_t chrram_size = chrrom_size ? 0 : CHR_BANK_SIZE;
	chr_size = chrrom_size ? chrrom_size : chrram_size;
	prgrom_size = ines.prgrom_nbanks * PRGROM_BANK_SIZE;
	sram_size = ines.sram_nbanks != 0
	            ? ines.sram_nbanks * SRAM_BANK_SIZE
	            : SRAM_BANK_SIZE;

	const uint32_t read_size = prgrom_size + chrrom_size;
	cartdata = malloc(read_size + chrram_size);
	memcpy(cartdata, &data[0x10], read_size);

	loginfo("INES HEADER:\n"
               "PRG-ROM BANKS: %" PRIi32 " x 16Kib = %" PRIi32 "\n"
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
	       "MAPPER: %" PRIu8 "\n\n",
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
	

	initmapper();
	return true;
}

void freerom(void)
{
	free(cartdata);
}
