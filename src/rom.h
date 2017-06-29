#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


typedef struct rom {
	long size;
	unsigned char data[];
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

	const char ines_match[5] = { 'N', 'E', 'S', 0x1A, '\0' };
	char ines[5] = { '\0', '\0', '\0', '\0', '\0' };
	fread(ines, 1, 4, file);
	if (strcmp(ines, ines_match) != 0) {
		fprintf(stderr, "\'%s\' is not an iNES file.\n", path);
		goto Lfclose;
	}

	fseek(file, 0, SEEK_END);
	const long filelen = ftell(file);
	fseek(file, 0, SEEK_SET);

	rom = malloc(sizeof(rom_t) + filelen);
	fread(rom->data, 1, filelen, file);
	rom->size = filelen;
Lfclose:
	fclose(file);
	return rom;

}


static inline void closerom(const rom_t* const rom)
{
	free((void*)rom);
}


#endif
