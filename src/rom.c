#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "cpu.h"
#include "ppu.h"
#include "rom.h"


static int_fast32_t prgrom_size;
static int_fast32_t chrrom_size;
static int_fast32_t chrram_size;
static int_fast32_t sram_size;
static enum MapperType mappertype;
static const char* rompath;
static const uint8_t* cartdata; // prgrom, chrrom
static uint8_t* sram;
static uint8_t* chrram;


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
	    memcmp(ines.match, match, sizeof(match)) != 0) {
		fprintf(stderr, "\'%s\' is not an ines file.\n", rompath);
		goto Lfclose;
	}
	
	// check cartridge compatibility
	mappertype = (ines.ctrl2&0xF0)|((ines.ctrl1&0xF0)>>4);
	if (mappertype > 1) {
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

	prgrom_size = ines.prgrom_nbanks * PRGROM_BANK_SIZE;
	if (ines.chrrom_nbanks > 0) {
		chrrom_size = ines.chrrom_nbanks * CHR_BANK_SIZE;
		chrram_size = 0;
		chrram = NULL;
	} else {
		chrram_size = CHR_BANK_SIZE;
		chrram = malloc(chrram_size);
		chrrom_size = 0;
	}

	sram_size = (ines.sram_nbanks != 0)
	            ? ines.sram_nbanks * SRAM_BANK_SIZE
	            : SRAM_BANK_SIZE;

	sram = calloc(sram_size, sizeof(char));

	const uint_fast32_t read_size = prgrom_size + chrrom_size;
	cartdata = malloc(read_size);
	fseek(file, 0x10, SEEK_SET);
	if (fread((void*)cartdata, 1, read_size, file) < read_size) {
		fprintf(stderr, "Couldn't read \'%s\' properly\n", rompath);
		freerom();
		goto Lfclose;
	}

	printf("PRG-ROM BANKS: %" PRIiFAST32 " x 16Kib = %" PRIiFAST32 "\n"
	       "CHR-ROM BANKS: %" PRIiFAST32 " x 8 Kib = %" PRIiFAST32 "\n"
	       "CHR-RAM BANKS: %" PRIiFAST32 " x 8 Kib = %" PRIiFAST32 "\n"
	       "SRAM BANKS:    %" PRIiFAST32 " x 8 Kib = %" PRIiFAST32 "\n"
	       "CTRL BYTE 1:\n"
	       "\tMIRRORING: %d = %s\n"
	       "\tBATTERY-BACKED RAM AT $6000-$7FFF: %d = %s\n"
	       "\tTRAINER AT $7000-71FF: %d = %s\n"
	       "\tFOUR SCREEN MIRRORING: %d = %s\n"
	       "\tFOUR LOWER BITS OF MAPPER NUMBER: $%.1x\n"
	       "CTRL BYTE 2:\n"
	       "\tBITS 0-3 RESERVED FOR FUTURE USE AND SHOULD ALL BE 0: $%.1x\n"
	       "\tFOUR UPPER BITS OF MAPPER NUMBER: $%.1x\n"
	       "MAPPER NUM %" PRIuFAST8 "\n",
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
	switch (mappertype) {
	case NROM:
		break;
	case MMC1:
		memset(&mapper.mmc1, 0, sizeof(mapper.mmc1));
		mapper.mmc1.reg[0] = 0x0C;
		break;
	}

	ret = true;

Lfclose:
	fclose(file);
	return ret;
}

void freerom(void)
{
	free((void*)cartdata);
	free(sram);
	free(chrram);
}


// NROM
static uint_fast8_t nrom_read(const uint_fast16_t addr)
{
	if (addr >= ADDR_PRGROM_UPPER && ines.prgrom_nbanks == 1)
		return cartdata[addr - ADDR_PRGROM_UPPER];
	return cartdata[addr - ADDR_PRGROM];
}

static void nrom_write(const uint_fast8_t value, const uint_fast16_t addr)
{
	((void)value);
	((void)addr);
	assert("NROM WRITE" && false);
}

static uint_fast8_t nrom_chrread(const uint_fast16_t addr)
{
	assert(addr < 0x2000);

	if (ines.chrrom_nbanks > 0)
		return cartdata[prgrom_size + addr];
	return chrram[addr];
}

static void nrom_chrwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	assert(addr < 0x2000);
	if (ines.chrrom_nbanks == 0)
		chrram[addr] = value;
}


// MMC1
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
	case 0x03:
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
			mapper.mmc1.reg[(addr&0x6000)>>13] = mapper.mmc1.tmp;
		}
	} else {
		mapper.mmc1.shiftcnt = 0;
		mapper.mmc1.reg[0] |= 0x0C;
	}
}

static uint_fast8_t mmc1_chrread(const uint_fast16_t addr)
{
	assert(addr < 0x2000);

	if (ines.chrrom_nbanks == 0)
		return chrram[addr];

	const uint8_t* const reg = mapper.mmc1.reg;
	const uint8_t* const chrrom = &cartdata[prgrom_size];
	if ((reg[0]&0x10) == 0) // switch 8kb banks at $0000 - $1FFF
		return chrrom[(reg[1]&0x1E) * 8192 + addr];
	else if (addr < 0x1000) // switch 4kb banks at $0000 - $0FFF
		return chrrom[reg[1] * 4096 + addr];
	else                    // switch 4kb banks at $1000 - $1FFF
		return chrrom[reg[2] * 4096 + (addr - 0x1000)];
}

static void mmc1_chrwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	assert(addr < 0x2000);
	if (ines.chrrom_nbanks == 0)
		chrram[addr] = value;
}



uint_fast8_t romread(const uint_fast16_t addr)
{
	if (addr >= ADDR_SRAM && addr < ADDR_PRGROM)
		return sram[addr - ADDR_SRAM];

	switch (mappertype) {
	case NROM: return nrom_read(addr); break;
	case MMC1: return mmc1_read(addr); break;
	}

	return 0;
}

void romwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	if (addr >= ADDR_SRAM && addr < ADDR_PRGROM) {
		sram[addr - ADDR_SRAM] = value;
		return;
	}

	switch (mappertype) {
	case NROM: nrom_write(value, addr); break;
	case MMC1: mmc1_write(value, addr); break;
	}
}

uint_fast8_t romchrread(const uint_fast16_t addr)
{
	switch (mappertype) {
	case NROM: return nrom_chrread(addr); break;
	case MMC1: return mmc1_chrread(addr); break;
	}
	return 0;
}

void romchrwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	switch (mappertype) {
	case NROM: nrom_chrwrite(value, addr); break;
	case MMC1: mmc1_chrwrite(value, addr); break;
	}
}

enum NTMirroring get_ntmirroring_mode(void)
{
	switch (mappertype) {
	case NROM:
		return (ines.ctrl1&0x01) ? NTMIRRORING_VERTICAL
		                         : NTMIRRORING_HORIZONTAL;
		break;
	case MMC1:
		switch (mapper.mmc1.reg[0]&0x03) {
		case 0x00: return NTMIRRORING_ONE_SCREEN_LOW;
		case 0x01: return NTMIRRORING_ONE_SCREEN_UPPER;
		case 0x02: return NTMIRRORING_VERTICAL;
		case 0x03: return NTMIRRORING_HORIZONTAL;
		}
		break;
	}

	return 0;
}

