#include <stdio.h>
#include "types.h"
#include "disassembler.h"


static inline const char* opstr(const uint8_t* data, int_fast32_t* offset);


char* disassemble(const rom_t* const rom)
{
	const int_fast32_t datasize = rom->prgrom_num_banks * PRGROM_BANK_SIZE;
	int_fast32_t offset = 0;

	while (offset < datasize) {
		const char* const str = opstr(rom->data, &offset);
		printf("%s\n", str);
	}

	return NULL;
}


static inline const char* opstr(const uint8_t* const data, int_fast32_t* const offset)
{
	const uint_fast8_t opcode = data[(*offset)++];

	switch (opcode) {
	// ADC
	case 0x69:
	case 0x65:
	case 0x75:
	case 0x6D:
	case 0x7D:
	case 0x79:
	case 0x61:
	case 0x71:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "ADC";

	// AND
	case 0x29:
	case 0x25:
	case 0x35:
	case 0x2D:
	case 0x3D:
	case 0x39:
	case 0x21:
	case 0x31:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "AND";

	// ASL
	case 0x0A:
	case 0x06:
	case 0x16:
	case 0x0E:
	case 0x1E:
		(*offset) += ((opcode&0x0F) == 0x0A) ? 0 :
		             ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "ASL";

	// BRANCH
	case 0x90: // BCC
	case 0xB0: // BCS
	case 0xF0: // BEQ
	case 0x30: // BMI
	case 0xD0: // BNE
	case 0x10: // BPL
	case 0x50: // BVC
	case 0x70: // BVS
		++(*offset);
		return "BRANCH";

	// STA
	case 0x85:
	case 0x95:
	case 0x8D:
	case 0x9D:
	case 0x99:
	case 0x81:
	case 0x91:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "STA";

	// CMP
	case 0xC9:
	case 0xC5:
	case 0xD5:
	case 0xCD:
	case 0xDD:
	case 0xD9:
	case 0xC1:
	case 0xD1:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "CMP"; 
	}

	return "UNKNOWN";
}


