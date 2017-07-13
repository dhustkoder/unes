#include <stdio.h>
#include "types.h"
#include "disassembler.h"


static inline void opstr(const uint8_t* data, int_fast32_t* offset, char buffer[16]);


void disassemble(const rom_t* const rom)
{
	const int_fast32_t datasize = rom->prgrom_num_banks * PRGROM_BANK_SIZE;
	int_fast32_t offset = 0;
	char buffer[16];
	while (offset < datasize) {
		opstr(rom->data, &offset, buffer);
		puts(buffer);
	}
}


static inline void opstr(const uint8_t* const data, int_fast32_t* const offset, char buffer[16])
{
	#define b16(msb, lsb)    (((msb)<<8)|(lsb))
	#define immediate(mn)   (sprintf(buffer, mn" #$%.2x", data[(*offset)++]))
	#define zeropage(mn)    (sprintf(buffer, mn" $%.2x", data[(*offset)++]))
	#define zeropagex(mn)   (sprintf(buffer, mn" $%.2x,X", data[(*offset)++]))
	#define indirectx(mn)   (sprintf(buffer, mn" ($%.2x,X)", data[(*offset)++]))
	#define indirecty(mn)   (sprintf(buffer, mn" ($%.2x),Y", data[(*offset)++]))
	#define absolute(mn)    (sprintf(buffer, mn" $%.4x", b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define absolutex(mn)   (sprintf(buffer, mn" $%.4x,X", b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define absolutey(mn)   (sprintf(buffer, mn" $%.4x,Y", b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define accumulator(mn) (sprintf(buffer, mn" A"))

	const uint_fast8_t opcode = data[(*offset)++];

	switch (opcode) {
	// ADC
	case 0x69: immediate("ADC"); break;
	case 0x65: zeropage("ADC");  break;
	case 0x75: zeropagex("ADC"); break;
	case 0x6D: absolute("ADC");  break;
	case 0x7D: absolutex("ADC"); break;
	case 0x79: absolutey("ADC"); break;
	case 0x61: indirectx("ADC"); break;
	case 0x71: indirecty("ADC"); break;

	// SBC
	case 0xE9: immediate("SBC"); break;
	case 0xE5: zeropage("SBC");  break;
	case 0xF5: zeropagex("SBC"); break;
	case 0xED: absolute("SBC");  break;
	case 0xFD: absolutex("SBC"); break;
	case 0xF9: absolutey("SBC"); break;
	case 0xE1: indirectx("SBC"); break;
	case 0xF1: indirecty("SBC"); break;

	// AND
	case 0x29: immediate("AND"); break;
	case 0x25: zeropage("AND");  break;
	case 0x35: zeropagex("AND"); break;
	case 0x2D: absolute("AND");  break;
	case 0x3D: absolutex("AND"); break;
	case 0x39: absolutey("AND"); break;
	case 0x21: indirectx("AND"); break;
	case 0x31: indirecty("AND"); break;

	// ORA (OR)
	case 0x09: immediate("ORA"); break;
	case 0x05: zeropage("ORA");  break;
	case 0x15: zeropagex("ORA"); break;
	case 0x0D: absolute("ORA");  break;
	case 0x1D: absolutex("ORA"); break;
	case 0x19: absolutey("ORA"); break;
	case 0x01: indirectx("ORA"); break;
	case 0x11: indirecty("ORA"); break;

	// EOR (XOR)
	case 0x49: immediate("EOR"); break;
	case 0x45: zeropage("EOR");  break;
	case 0x55: zeropagex("EOR"); break;
	case 0x4D: absolute("EOR");  break;
	case 0x5D: absolutex("EOR"); break;
	case 0x59: absolutey("EOR"); break;
	case 0x41: indirectx("EOR"); break;
	case 0x51: indirecty("EOR"); break;

	// ASL
	case 0x0A: accumulator("ASL"); break;
	case 0x06: zeropage("ASL");    break;
	case 0x16: zeropagex("ASL");   break;
	case 0x0E: absolute("ASL");    break;
	case 0x1E: absolutex("ASL");   break;

	// LSR
	case 0x4A: accumulator("LSR"); break;
	case 0x46: zeropage("LSR");    break;
	case 0x56: zeropagex("LSR");   break;
	case 0x4E: absolute("LSR");    break;
	case 0x5E: absolutex("LSR");   break;

	// ROL
	case 0x2A: accumulator("ROL"); break;
	case 0x26: zeropage("ROL");    break;
	case 0x36: zeropagex("ROL");   break;
	case 0x2E: absolute("ROL");    break;
	case 0x3E: absolutex("ROL");   break;

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
		sprintf(buffer, "BRANCH");
		break;
	// INC
	case 0xE6:
	case 0xF6:
	case 0xEE:
	case 0xFE:
		(*offset) += ((opcode&0x0F) == 0x0E) ? 2 : 1;
		sprintf(buffer, "INC");
		break;
	// DEC
	case 0xC6:
	case 0xD6:
	case 0xCE:
	case 0xDE:
		(*offset) += ((opcode&0x0F) == 0x0E) ? 2 : 1;
		sprintf(buffer, "DEC");
		break;
	// LDA
	case 0xA9:
	case 0xA5:
	case 0xB5:
	case 0xAD:
	case 0xBD:
	case 0xB9:
	case 0xA1:
	case 0xB1:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		sprintf(buffer, "LDA");
		break;
	// LDX
	case 0xA2:
	case 0xA6:
	case 0xB6:
	case 0xAE:
	case 0xBE:
		(*offset) += ((opcode&0x0F) == 0x0E) ? 2 : 1;
		sprintf(buffer, "LDX");
		break;
	// LDY
	case 0xA0:
	case 0xA4:
	case 0xB4:
	case 0xAC:
	case 0xBC:
		(*offset) += ((opcode&0x0F) == 0x0C) ? 2 : 1;
		sprintf(buffer, "LDY");
		break;
	// STA
	case 0x85:
	case 0x95:
	case 0x8D:
	case 0x9D:
	case 0x99:
	case 0x81:
	case 0x91:
		(*offset) += ((opcode&0x0F) >= 0x09) ? 2 : 1;
		sprintf(buffer, "STA");
		break;
	// STX
	case 0x86:
	case 0x96:
	case 0x8E:
		(*offset) += (opcode == 0x8E) ? 2 : 1;
		sprintf(buffer, "STX");
		break;
	// STY
	case 0x84:
	case 0x94:
	case 0x8C:
		(*offset) += (opcode == 0x8C) ? 2 : 1;
		sprintf(buffer, "STY");
		break;
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
		sprintf(buffer, "CMP");
		break;
	// ROR
	case 0x6A:
	case 0x66:
	case 0x76:
	case 0x6E:
	case 0x7E:
		(*offset) += opcode == 0x6A ? 0 :
		             ((opcode&0x0F) == 0x0E) ? 2 : 1;
		sprintf(buffer, "ROR");
		break;
	// BIT
	case 0x24:
	case 0x2C:
		(*offset) += (opcode == 0x2C) ? 2 : 1;
		sprintf(buffer, "BIT");
		break;
	// CPX
	case 0xE0:
	case 0xE4:
	case 0xEC:
		(*offset) += (opcode == 0xEC) ? 2 : 1;
		sprintf(buffer, "CPX");
		break;
	// CPY
	case 0xC0:
	case 0xC4:
	case 0xCC:
		(*offset) += (opcode == 0xCC) ? 2 : 1;
		sprintf(buffer, "CPY");
		break;
	// JSR
	case 0x20:
		(*offset) += 2;
		sprintf(buffer, "JSR");
		break;
	// JMP
	case 0x4C:
	case 0x6C:
		(*offset) += 2;
		sprintf(buffer, "JMP");
		break;
	// BRK
	case 0x00: sprintf(buffer, "BRK"); break;
	// CLC
	case 0x18: sprintf(buffer, "CLC"); break;
	// CLI
	case 0x58: sprintf(buffer, "CLI"); break;
	// SEI
	case 0x78: sprintf(buffer, "SEI"); break;
	// CLV
	case 0xB8: sprintf(buffer, "CLV"); break;
	// CLD
	case 0xD8: sprintf(buffer, "CLD"); break;
	// SED
	case 0xF8: sprintf(buffer, "SED"); break;
	// DEX
	case 0xCA: sprintf(buffer, "DEX"); break;
	// DEY
	case 0x88: sprintf(buffer, "DEY"); break;
	// INX
	case 0xE8: sprintf(buffer, "INX"); break;
	// INY
	case 0xC8: sprintf(buffer, "INY"); break;
	// PHP
	case 0x08: sprintf(buffer, "PHP"); break;
	// PLP
	case 0x28: sprintf(buffer, "PLP"); break;
	// PHA
	case 0x48: sprintf(buffer, "PHA"); break;
	// PLA
	case 0x68: sprintf(buffer, "PLA"); break;
	// NOP
	case 0xEA: sprintf(buffer, "NOP"); break;
	// RTI
	case 0x40: sprintf(buffer, "RTI"); break;
	// RTS
	case 0x60: sprintf(buffer, "RTS"); break;
	// SEC
	case 0x38: sprintf(buffer, "SEC"); break;
	// TAX
	case 0xAA: sprintf(buffer, "TAX"); break;
	// TXA
	case 0x8A: sprintf(buffer, "TXA"); break;
	// TAY
	case 0xA8: sprintf(buffer, "TAY"); break;
	// TSX
	case 0xBA: sprintf(buffer, "TSX"); break;
	// TXS
	case 0x9A: sprintf(buffer, "TXS"); break;
	// TYA
	case 0x98: sprintf(buffer, "TYA"); break;  

	default: sprintf(buffer, "UNKNOWN"); break;
	}
}


