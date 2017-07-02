#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>


#define PRGROM_BANK_SIZE ((int_fast32_t)16384)
#define VROM_BANK_SIZE   ((int_fast32_t)8192)
#define RAM_BANK_SIZE    ((int_fast32_t)8192)


typedef struct rom {
	uint8_t prgrom_num_banks;
	uint8_t vrom_num_banks;
	uint8_t ram_num_banks;	
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t data[];
} rom_t;


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

	const int_fast32_t datasize = ines_header[4] * PRGROM_BANK_SIZE +
	                    ines_header[5] * VROM_BANK_SIZE +
			    ines_header[8] * RAM_BANK_SIZE;

	rom = malloc(sizeof(rom_t) + datasize); 
	rom->prgrom_num_banks = ines_header[4];
	rom->vrom_num_banks = ines_header[5];
	rom->ctrl1 = ines_header[6];
	rom->ctrl2 = ines_header[7];
	rom->ram_num_banks = ines_header[8];
	fread(rom->data, 1, datasize, file);
Lfclose:
	fclose(file);
	return rom;

}


static inline void closerom(const rom_t* const rom)
{
	free((void*)rom);
}



#endif
