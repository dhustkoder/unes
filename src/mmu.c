#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "mmu.h"


enum RomType {
	NROM = 0x00,
	MMC1 = 0x01
};

// rom chips registers
static union {
	struct {
		uint_fast8_t r0;
		uint_fast8_t r1;
		uint_fast8_t r2;
		uint_fast8_t r3;
		uint_fast8_t tmp;
		int_fast8_t shift_n;
	} mmc1;
} map;


static int_fast32_t prg_size;
static int_fast32_t chr_size;
static int_fast32_t cart_ram_size;
static uint_fast8_t romtype;
static const char* rompath;

static uint8_t ines[0x10]   = { 0 };   // ines header
static const uint8_t* cartdata;        // prgrom,chrdata,cart_ram
static uint8_t ram[0x800]   = { 0 };   // zeropage,stack,ram
uint8_t io[0x28]            = { 0 };   // all io registers


static void romwrite(const uint_fast8_t val, const int_fast32_t addr)
{
	switch (romtype) {
	case NROM:
		break;

	case MMC1:
		if ((val&0x80) == 0x80)
			map.mmc1.shift_n = 0;

		map.mmc1.tmp |= ((val&0x01)<<map.mmc1.shift_n);

		if ((++map.mmc1.shift_n) == 0x04) {
			map.mmc1.shift_n = 0;
			if (addr >= 0xE000)
				map.mmc1.r3 = map.mmc1.tmp;
			else if (addr >= 0xC000)
				map.mmc1.r2 = map.mmc1.tmp;
			else if (addr >= 0xA000)
				map.mmc1.r1 = map.mmc1.tmp;
			else if (addr >= 0x8000)
				map.mmc1.r0 = map.mmc1.tmp;
			map.mmc1.tmp = 0x00;
		}

		break;

	default:
		fprintf(stderr, "Mapper write not supported.\n");
		exit(EXIT_FAILURE);
	}
}

static uint_fast8_t romread(const int_fast32_t addr)
{
	switch (romtype) {
	case NROM:
		if (addr >= ADDR_PRGROM_UPPER && ines[4] == 1)
			return cartdata[addr - ADDR_PRGROM_UPPER];
		return cartdata[addr - ADDR_PRGROM];
	}
	
	fprintf(stderr, "Mapper read not supported.\n");
	exit(EXIT_FAILURE);
}


static bool loadrom(const char* const path)
{
	bool ret = false;

	rompath = path;
	FILE* const file = fopen(rompath, "r");

	if (file == NULL) {
		fprintf(stderr, "Couldn't open rom \'%s\': %s\n",
		        rompath, strerror(errno));
		return false;
	}

	const uint8_t ines_match[] = { 'N', 'E', 'S', 0x1A };
	fread(ines, 1, 0x10, file);
	if (memcmp(ines, ines_match, sizeof(ines_match)) != 0) {
		fprintf(stderr, "\'%s\' is not an ines file.\n", rompath);
		goto Lfclose;
	}

	romtype = (ines[7]&0xF0)|((ines[6]&0xF0)>>0x04);
	prg_size = ines[4] * PRGROM_BANK_SIZE;
	chr_size = ines[5] * CHR_BANK_SIZE;
	cart_ram_size  = (ines[8] != 0) ? ines[8] * CART_RAM_BANK_SIZE : CART_RAM_BANK_SIZE;
	const uint_fast32_t read_size = ((ines[6]&0x04) ? TRAINER_SIZE : 0) + prg_size + chr_size;

	cartdata = malloc(read_size + cart_ram_size); 
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
	       ines[4], prg_size,
	       ines[5], chr_size,
	       ines[8], cart_ram_size,
	       (ines[6]&0x01), (ines[6]&0x01) ? "VERTICAL" : "HORIZONTAL",
	       (ines[6]&0x02)>>1, (ines[6]&0x02) ? "YES" : "NO",
	       (ines[6]&0x04)>>2, (ines[6]&0x04) ? "YES" : "NO",
	       (ines[6]&0x08)>>3, (ines[6]&0x08) ? "YES" : "NO",
	       (ines[6]&0xF0)>>4, ines[7]&0x0F, (ines[7]&0xF0)>>4, romtype);

	ret = true;
	
Lfclose:
	fclose(file);
	return ret;
}


bool initmmu(const char* const rompath)
{
	return loadrom(rompath);
}

void termmmu(void)
{
	free((void*)cartdata);
}


uint_fast8_t mmuread(const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF]; // also handles mirrors
	else if (addr >= ADDR_PRGROM)
		return romread(addr);
	else if (addr >= ADDR_IOREGS2 && addr < ADDR_EXPROM)
		return io[0x08 + (addr - ADDR_IOREGS2)]; // IOREGS2
	else if (addr >= ADDR_IOREGS1 && addr < ADDR_IOREGS2)
		return io[(addr - ADDR_IOREGS1)&0x08]; // also handles mirrors

	return 0;
}


void mmuwrite(const uint_fast8_t value, const int_fast32_t addr)
{
	assert(addr <= 0xFFFF);

	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = value; // also handles mirrors
	else if (addr < ADDR_IOREGS2)
		io[(addr - ADDR_IOREGS1)&0x08] = value; // also handles mirrors
	else if (addr < ADDR_EXPROM)
		io[0x08 + (addr - ADDR_IOREGS2)] = value;
	else if (addr >= ADDR_PRGROM)
		romwrite(value, addr);
}

