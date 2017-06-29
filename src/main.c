#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "rom.h"


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

