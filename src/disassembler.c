#include <stdio.h>
#include "types.h"
#include "disassembler.h"


static inline const char* opstr(const uint8_t* data, int_fast32_t* offset);


void disassemble(const rom_t* const rom)
{
	const int_fast32_t datasize = rom->prgrom_num_banks * PRGROM_BANK_SIZE;
	int_fast32_t offset = 0;

	while (offset < datasize)
		printf("%s\n", opstr(rom->data, &offset));
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

	// ORA (OR)
	case 0x09:
	case 0x05:
	case 0x15:
	case 0x0D:
	case 0x1D:
	case 0x19:
	case 0x01:
	case 0x11:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "ORA";

	// EOR (XOR)
	case 0x49:
	case 0x45:
	case 0x55:
	case 0x4D:
	case 0x5D:
	case 0x59:
	case 0x41:
	case 0x51:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		return "EOR";

	// ASL
	case 0x0A:
	case 0x06:
	case 0x16:
	case 0x0E:
	case 0x1E:
		(*offset) += ((opcode&0x0F) == 0x0A) ? 0 :
		             ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "ASL";
	// LSR
	case 0x4A:
	case 0x46:
	case 0x56:
	case 0x4E:
	case 0x5E:
		(*offset) += ((opcode&0x0F) == 0x0A) ? 0 :
		             ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "LSR";
	// ROL
	case 0x2A:
	case 0x26:
	case 0x36:
	case 0x2E:
	case 0x3E:
		(*offset) += ((opcode&0x0F) == 0x0A) ? 0 :
		             ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "ROL";

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

	// INC
	case 0xE6:
	case 0xF6:
	case 0xEE:
	case 0xFE:
		(*offset) += ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "INC";

	// DEC
	case 0xC6:
	case 0xD6:
	case 0xCE:
	case 0xDE:
		(*offset) += ((opcode&0x0F) == 0x0E) ? 2 : 1;
		return "DEC";

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
	
	// BIT
	case 0x24:
	case 0x2C:
		(*offset) += (opcode == 0x2C) ? 2 : 1;
		return "BIT";

	// CPX
	case 0xE0:
	case 0xE4:
	case 0xEC:
		(*offset) += (opcode == 0xEC) ? 2 : 1;
		return "CPX";

	// CPY
	case 0xC0:
	case 0xC4:
	case 0xCC:
		(*offset) += (opcode == 0xCC) ? 2 : 1;
		return "CPY";
	
	// JSR
	case 0x20:
		(*offset) += 2;
		return "JSR";
	
	// JMP
	case 0x4C:
	case 0x6C:
		(*offset) += 2;
		return "JMP";

	// BRK
	case 0x00: return "BRK";
	// CLC
	case 0x18: return "CLC";
	// CLI
	case 0x58: return "CLI";
	// SEI
	case 0x78: return "SEI";
	// CLV
	case 0xB8: return "CLV";
	// CLD
	case 0xD8: return "CLD";
	// SED
	case 0xF8: return "SED";
	// DEX
	case 0xCA: return "DEX";
	// DEY
	case 0x88: return "DEY";	
	// INX
	case 0xE8: return "INX";
	// INY
	case 0xC8: return "INY";
	}

	return "UNKNOWN";
}


