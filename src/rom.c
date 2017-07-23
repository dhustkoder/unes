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

static uint8_t ines[0x10]   = { 0 };   // ines header
static const uint8_t* cartdata;        // prgrom,chrdata,cart_ram



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
	const uint_fast32_t read_size = prg_size + chr_size /*((ines[6]&0x04) ? TRAINER_SIZE : 0) + */;

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

	
	ppu_load_chr_rom(&cartdata[prg_size]);

	ret = true;
Lfclose:
	fclose(file);
	return ret;
}

void freerom(void)
{
	free((void*)cartdata);
}


uint_fast8_t romread(const int_fast32_t addr)
{
	if (addr >= ADDR_PRGROM_UPPER && ines[4] == 1)
		return cartdata[addr - ADDR_PRGROM_UPPER];
	return cartdata[addr - ADDR_PRGROM];
}
