#include "rom.h"
#include "disassembler.h"


static inline const char* opstr(const uint8_t data[], int_fast32_t *offset)
{
	const char* str;

	switch (data[++(*offset)]) {
	default: str = "UNKNOWN"; break;
	case 0x09: str = "ORA #"; ++(*offset); break;
	case 0x0B: str = "ANC #"; ++(*offset); break;
	case 0x29: str = "AND #"; ++(*offset); break;
	case 0x2B: str = "ANC #"; ++(*offset); break;
	case 0x49: str = "EOR #"; ++(*offset); break;
	case 0x4B: str = "ALR #"; ++(*offset); break;
	case 0x69: str = "ADC #"; ++(*offset); break;
	case 0x6B: str = "ARR #"; ++(*offset); break;
	case 0x80: str = "NOP #"; ++(*offset); break;
	case 0x82: str = "NOP #"; ++(*offset); break;
	case 0x89: str = "NOP #"; ++(*offset); break;
	case 0x8B: str = "XAA #"; ++(*offset); break;
	case 0xA0: str = "LDY #"; ++(*offset); break;
	case 0xA2: str = "LDX #"; ++(*offset); break;
	case 0xA9: str = "LDA #"; ++(*offset); break;
	case 0xAB: str = "LAX #"; ++(*offset); break;
	case 0xC0: str = "CPY #"; ++(*offset); break;
	case 0xC2: str = "NOP #"; ++(*offset); break;
	case 0xC9: str = "CMP #"; ++(*offset); break;
	case 0xCB: str = "AXS #"; ++(*offset); break;
	case 0xE0: str = "CPX #"; ++(*offset); break;
	case 0xE2: str = "NOP #"; ++(*offset); break;
	case 0xE9: str = "SBC #"; ++(*offset); break;
	case 0xEB: str = "SBC #"; ++(*offset); break;
	}

	return str;
}


char* disassemble(const rom_t* const rom)
{
	int_fast32_t datasize = rom->prgrom_num_banks * PRGROM_BANK_SIZE;
	int_fast32_t offset = 0;

	while (offset < datasize) {
		const char* const str = opstr(rom->data, &offset);
		printf("%s\n", str);
	}

	return NULL;
}

