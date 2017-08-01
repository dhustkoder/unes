#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include "mmu.h"
#include "ppu.h"
#include "rom.h"


static int_fast32_t prg_size;
static int_fast32_t chr_size;
static int_fast32_t cart_ram_size;
static uint_fast8_t romtype;
static const char* rompath;
static const uint8_t* cartdata;  // prgrom,chrdata,cart ram


// support only NROM for now
static struct {
	char match[4];
	uint8_t prgrom_nbanks;  // number of 16 kib prg-rom banks.
	uint8_t chrrom_nbanks;  // nuber of 8 kib vrom banks
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t ram_nbanks;     // number of 8 kib ram banks
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
		fprintf(stderr, "Couldn't open rom \'%s\': %s\n",
		        rompath, strerror(errno));
		return false;
	}

	static const uint8_t match[] = { 'N', 'E', 'S', 0x1A };

	if (fread(&ines, 1, 9, file) < 9 ||
	    memcmp(ines.match, match, sizeof(match)) != 0) {
		fprintf(stderr, "\'%s\' is not an ines file.\n", rompath);
		goto Lfclose;
	}

	romtype = (ines.ctrl2&0xF0)|((ines.ctrl1&0xF0)>>4);

	if (romtype > 1) {
		fprintf(stderr, "\'%s\': mapper %d not supported.\n", rompath, romtype);
		goto Lfclose;
	} else if ((ines.ctrl1&0x04) != 0) {
		fprintf(stderr, "\'%s\': trainer is not supported.\n", rompath);
		goto Lfclose;
	}

	prg_size = ines.prgrom_nbanks * PRGROM_BANK_SIZE;
	chr_size = ines.chrrom_nbanks * CHR_BANK_SIZE;
	cart_ram_size  = (ines.ram_nbanks != 0)
	                 ? ines.ram_nbanks * CART_RAM_BANK_SIZE
	                 : CART_RAM_BANK_SIZE;

	const uint_fast32_t read_size = prg_size + chr_size;
	cartdata = malloc(read_size + cart_ram_size); 
	fseek(file, 0x10, SEEK_SET);
	if (fread((uint8_t*)cartdata, 1, read_size, file) < read_size) {
		fprintf(stderr, "Couldn't read \'%s\' properly\n", rompath);
		free((void*)cartdata);
		goto Lfclose;
	}

	printf("PRG-ROM BANKS: %" PRIu8 " x 16Kib = %" PRIiFAST32 "\n"
	       "VROM BANKS: %" PRIu8 " x 8 Kib = %" PRIiFAST32 "\n"
	       "RAM BANKS: %" PRIu8 " x 8 Kib = %" PRIiFAST32 "\n"
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
	       ines.prgrom_nbanks, prg_size,
	       ines.chrrom_nbanks, chr_size,
	       ines.ram_nbanks, cart_ram_size,
	       (ines.ctrl1&0x01), (ines.ctrl1&0x01) ? "VERTICAL" : "HORIZONTAL",
	       (ines.ctrl1&0x02)>>1, (ines.ctrl1&0x02) ? "YES" : "NO",
	       (ines.ctrl1&0x04)>>2, (ines.ctrl1&0x04) ? "YES" : "NO",
	       (ines.ctrl1&0x08)>>3, (ines.ctrl1&0x08) ? "YES" : "NO",
	       (ines.ctrl1&0xF0)>>4, ines.ctrl2&0x0F, (ines.ctrl2&0xF0)>>4, romtype);

	
	ret = true;

Lfclose:
	fclose(file);
	return ret;
}

void freerom(void)
{
	free((void*)cartdata);
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
	printf("NROM WRITE: %x to %lx\n", value, addr);
}


// MMC1
static uint_fast8_t mmc1_read(const uint_fast16_t addr)
{
	const uint_fast8_t reg3 = mapper.mmc1.reg[3];
	uint_fast16_t offset;
	switch ((mapper.mmc1.reg[0]&0x0C)>>2) {
	case 0x00:
	case 0x01:
		offset = (PRGROM_BANK_SIZE * 2 * (reg3&0x0E)) + (addr - ADDR_PRGROM);
		break;
	case 0x02:
		if (addr < ADDR_PRGROM_UPPER) {
			offset = addr - ADDR_PRGROM;
			break;
		}
		offset = (PRGROM_BANK_SIZE * (reg3&0x0F)) + addr - ADDR_PRGROM_UPPER;
		break;
	case 0x03:
		if (addr >= ADDR_PRGROM_UPPER) {
			offset = ((ines.prgrom_nbanks - 1) * PRGROM_BANK_SIZE) + (addr - ADDR_PRGROM_UPPER);
			break;
		}
		offset = (PRGROM_BANK_SIZE * (reg3&0x0F)) + (addr - ADDR_PRGROM);
		break;
	}

	return cartdata[offset];
}

static void mmc1_write(const uint_fast8_t value, const uint_fast16_t addr)
{
	printf("MMC1 WRITE: $%x to $%lx\n", value, addr);

	if ((value&0x80) == 0) {
		mapper.mmc1.tmp >>= 1;
		mapper.mmc1.tmp |= (value&0x01)<<4;
		if (++mapper.mmc1.shiftcnt == 5) {
			mapper.mmc1.shiftcnt = 0;

			unsigned n;
			if (addr <= 0x9FFF)
				n = 0;
			else if (addr <= 0xBFFF)
				n = 1;
			else if (addr <= 0xDFFF)
				n = 2;
			else
				n = 3;

			mapper.mmc1.reg[n] = mapper.mmc1.tmp;
			mapper.mmc1.tmp = 0;
			printf("REG %u evals %"PRIuFAST8"\n", 
			       n, mapper.mmc1.reg[n]);
		}
	} else {
		mapper.mmc1.shiftcnt = 0;
		mapper.mmc1.tmp = 0;
		mapper.mmc1.reg[0] |= 0x0C;
	}

}




uint_fast8_t romread(const uint_fast16_t addr)
{
	switch (romtype) {
	case 0x00: return nrom_read(addr);
	case 0x01: return mmc1_read(addr);
	}
	return 0;
}

void romwrite(const uint_fast8_t value, const uint_fast16_t addr)
{
	switch (romtype) {
	case 0x00: nrom_write(value, addr); break;
	case 0x01: mmc1_write(value, addr); break;
	}
}

