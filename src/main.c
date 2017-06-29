#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
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


static inline void printascii(const unsigned char* const data, long begin, long end)
{
	printf("................: ");

	for (long i = begin; i < end; ++i) {
		const char byte = (char) data[i];
		if (isalnum(byte))
			printf("%c ", byte);
		else
			printf(". ");
	}

	putchar('\n');
}

int main(const int argc, const char* const * const argv)
{
	if (argc > 1) {
		const rom_t* const rom = openrom(argv[1]);

		if (rom == NULL)
			return EXIT_FAILURE;

		printf("ROM SIZE: %ld\n", rom->size);

		long b;
		for (b = 0; b < rom->size; ++b) {
			if ((b % 8) != 0) {
				printf("%.2X ", rom->data[b]);
			} else if (b >= 8) {
				printf("%.2X", rom->data[b]);
				printascii(rom->data, b - 8, b);
			}
		}

		const long diff = --b % 8;
		if (diff != 0) {
			for (long i = diff; i < 8; ++i)
				putchar(' ');
			printascii(rom->data, b - diff, b);
		}

		closerom(rom);
		return EXIT_SUCCESS;
	}

	fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
	return EXIT_FAILURE;
}

