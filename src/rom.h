#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "types.h"


#define PRGROM_BANK_SIZE ((int_fast32_t)16384)
#define VROM_BANK_SIZE   ((int_fast32_t)8192)
#define RAM_BANK_SIZE    ((int_fast32_t)8192)
#define TRAINER_SIZE     ((int_fast32_t)512)


static inline rom_t* openrom(const char* const path)
{
	rom_t* rom = NULL;

	FILE* const file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "Couldn't open rom \'%s\': %s\n",
		        path, strerror(errno));
		return NULL;
	}

	const uint8_t header_match[] = { 'N', 'E', 'S', 0x1A };
	uint8_t ines_header[16];
	fread(ines_header, 1, 16, file);
	if (memcmp(ines_header, header_match, sizeof(header_match)) != 0) {
		fprintf(stderr, "\'%s\' is not an ines file.\n", path);
		goto Lfclose;
	}

	const int_fast32_t prg_size = ines_header[4] * PRGROM_BANK_SIZE;
	const int_fast32_t vrom_size = ines_header[5] * VROM_BANK_SIZE;
	const int_fast32_t ram_size  = (ines_header[8] != 0) ? ines_header[8] * RAM_BANK_SIZE : RAM_BANK_SIZE;
	const int_fast32_t trainer_size = (ines_header[6]&0x04) ? TRAINER_SIZE : 0;
	const uint_fast32_t read_size = trainer_size + prg_size + vrom_size;

	rom = malloc(sizeof(rom_t) + read_size + ram_size); 
	if (fread(rom->data, 1, read_size, file) < read_size) {
		fprintf(stderr, "Couldn't read file \'%s\' properly.\n", path);
		free(rom);
		goto Lfclose;
	}

	rom->prgrom_num_banks = ines_header[4];
	rom->vrom_num_banks = ines_header[5];
	rom->ctrl1 = ines_header[6];
	rom->ctrl2 = ines_header[7];
	rom->ram_num_banks = ines_header[8];

Lfclose:
	fclose(file);
	return rom;
}


static inline void closerom(const rom_t* const rom)
{
	free((void*)rom);
}


#endif
