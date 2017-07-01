#include <stdio.h>
#include <stdlib.h>
#include "rom.h"



int main(const int argc, const char* const * const argv)
{
	if (argc > 1) {
		const rom_t* const rom = openrom(argv[1]);

		if (rom == NULL)
			return EXIT_FAILURE;

		printf("PRG-ROM BANKS: %d x 16Kib = %u\n"
		       "VROM BANKS: %d x 8 Kib = %u\n"
		       "RAM BANKS: %d x 8 Kib = %u\n"
		       "CTRL BYTE 1:\n"
		       "\tMIRRORING: %d = %s\n"
		       "\tBATTERY-BACKED RAM AT $6000-$7FFF: %d = %s\n"
		       "\tTRAINER AT $7000-71FF: %d = %s\n"
		       "\tFOUR SCREEN MIRRORING: %d = %s\n"
		       "\tFOUR LOWER BITS OF MAPPER NUMBER: $%.1x\n"
		       "CTRL BYTE 2:\n"
		       "\tBITS 0-3 RESERVED FOR FUTURE USE AND SHOULD ALL BE 0: $%.1x\n"
		       "\tFOUR UPPER BITS OF MAPPER NUMBER: $%.1x\n",
		       rom->prgrom_num_banks, rom->prgrom_num_banks * PRGROM_BANK_SIZE,
		       rom->vrom_num_banks, rom->vrom_num_banks * VROM_BANK_SIZE,
		       rom->ram_num_banks, rom->ram_num_banks * RAM_BANK_SIZE,
		       (rom->ctrl1&0x01), (rom->ctrl1&0x01) ? "VERTICAL" : "HORIZONTAL",
		       (rom->ctrl1&0x02)>>1, (rom->ctrl1&0x02) ? "YES" : "NO",
		       (rom->ctrl1&0x04)>>2, (rom->ctrl1&0x04) ? "YES" : "NO",
		       (rom->ctrl1&0x08)>>3, (rom->ctrl1&0x08) ? "YES" : "NO",
		       (rom->ctrl1&0xF0)>>4, rom->ctrl2&0x0F, (rom->ctrl2&0xF0)>>4);

		closerom(rom);
		return EXIT_SUCCESS;
	}

	fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
	return EXIT_FAILURE;
}

