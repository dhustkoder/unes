#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [binary file]\n", argv[1]);
		return EXIT_FAILURE;
	}

	FILE* const file = fopen(argv[1], "r");

	fseek(file, 0, SEEK_END);
	const long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* const data = malloc(size);
	fread(data, 1, size, file);

	fclose(file);

	printf("static const uint8_t binary[] = {\n\t");

	for (long i = 0, brk = 0; i < size; ++i, ++brk) {
		if (brk == 32) {
			printf("\n\t");
			brk = 0;
		}

		if (i < size - 1)
			printf("0x%.2X, ", data[i]);
		else
			printf("0x%.2X\n};", data[i]);
	}

	free(data);
	return EXIT_SUCCESS;

}
