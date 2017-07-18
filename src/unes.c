#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "rom.h"
#include "disassembler.h"
#include "mem.h"
#include "cpu.h"


static int unes(const char* const path, const bool disass)
{
	rom_t* const rom = openrom(path);

	if (rom == NULL)
		return EXIT_FAILURE;

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
	       "\tFOUR UPPER BITS OF MAPPER NUMBER: $%.1x\n",
	       rom->prgrom_num_banks, rom->prgrom_num_banks * PRGROM_BANK_SIZE,
	       rom->vrom_num_banks, rom->vrom_num_banks * VROM_BANK_SIZE,
	       rom->ram_num_banks, rom->ram_num_banks * RAM_BANK_SIZE,
	       (rom->ctrl1&0x01), (rom->ctrl1&0x01) ? "VERTICAL" : "HORIZONTAL",
	       (rom->ctrl1&0x02)>>1, (rom->ctrl1&0x02) ? "YES" : "NO",
	       (rom->ctrl1&0x04)>>2, (rom->ctrl1&0x04) ? "YES" : "NO",
	       (rom->ctrl1&0x08)>>3, (rom->ctrl1&0x08) ? "YES" : "NO",
	       (rom->ctrl1&0xF0)>>4, rom->ctrl2&0x0F, (rom->ctrl2&0xF0)>>4);

	if (disass) {
		disassemble(rom);
	} else {
		initmem(rom);
		initcpu();

		for (;;) {
			stepcpu();
		}
	}

	closerom(rom);
	return EXIT_SUCCESS;
}


int main(const int argc, const char* const * const argv)
{
	if (argc > 1) {
		const char* path = NULL;
		bool disassemble = false;

		for (int i = 1; i < argc; ++i) {
			if (argv[i][0] != '-') {
				if (path == NULL) {
					path = argv[i];
				} else {
					path = NULL;
					break;
				}
			} else {
				if (strcmp(argv[i], "-d") == 0)
					disassemble = true;
			}
		}

		if (path != NULL)
			return unes(path, disassemble);
	}

	fprintf(stderr, "Usage: %s [rom path] [-d disassemble]\n", argv[0]);
	return EXIT_FAILURE;
}
