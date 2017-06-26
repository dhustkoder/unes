#include <stdio.h>
#include <stdlib.h>


struct rom {
	long size;
	unsigned char data[];
};

typedef struct rom rom_t;


static inline rom_t* openrom(const char* const path)
{
	FILE* const file = fopen(path, "r");

	if (file == NULL) {
		perror("Couldn't open rom");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	const long filelen = ftell(file);
	fseek(file, 0, SEEK_SET);

	rom_t* const rom = malloc(sizeof(rom_t) + filelen);

	if (rom == NULL) {
		perror("Couldn't allocate memory");
		goto Lfclose;
	}

	rom->size = filelen;
	fread(rom->data, 1, filelen, file);
Lfclose:
	fclose(file);
	return rom;
}


static inline void closerom(const rom_t* const rom)
{
	free((void*)rom);
}


int main(const int argc, const char* const * const argv)
{
	if (argc > 1) {
		const rom_t* const rom = openrom(argv[1]);

		if (rom == NULL)
			return EXIT_FAILURE;

		fwrite(rom->data, 1, rom->size, stdout);
		putchar('\n');

		closerom(rom);
		return EXIT_SUCCESS;
	}

	fprintf(stderr, "Usage: %s [rom path]\n", argv[0]);
	return EXIT_FAILURE;
}

