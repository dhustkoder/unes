#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "types.h"
#include "disassembler.h"


static inline void opstr(const uint8_t* data,
                         int_fast32_t* offset,
                         int_fast32_t labels[128],
                         int* labels_idx,
                         char buffer[32]);


void disassemble(const rom_t* const rom)
{
	const int_fast32_t datasize = rom->prgrom_num_banks * PRGROM_BANK_SIZE;
	int_fast32_t offset = 0;
	int_fast32_t labels[128];
	int labels_idx = 0;
	char buffer[32];

	while (offset < datasize) {
		opstr(rom->data, &offset, labels, &labels_idx, buffer);
		puts(buffer);
	}
}


static void branch_op(const char* const mnemonic,
                      const uint8_t* const data,
                      const int_fast32_t offset,
		      const int_fast32_t labels[],
		      const int* const labels_idx,
		      char buffer[])
{
	const int_fast32_t target = offset + 1 + ((int8_t)data[offset]);

	if (labels != NULL) {
		bool islabel = false;

		if (target < (offset + 1)) {
			int idx = *labels_idx - 1;
			while (idx >= 0 && labels[idx] > target)
				--idx;
			if (idx >= 0 && labels[idx] == target)
				islabel = true;
		} else {
			int_fast32_t p = offset + 1;
			while (p < target)
				opstr(data, &p, NULL, NULL, buffer);
			if (p == target)
				islabel = true;
		}

		if (!islabel) {
			sprintf(buffer, ".hex %x %x", data[offset - 1], data[offset]);
			return;
		}
	}

	sprintf(buffer, "%s B%"PRIdFAST32"_%.4lx", mnemonic, offset / 0x4000, target);
}


static inline void opstr(const uint8_t* const data,
                         int_fast32_t* const offset,
			 int_fast32_t labels[128],
			 int* const labels_idx,
			 char buffer[32])
{
	#define b16(msb, lsb)    (((msb)<<8)|(lsb))
	#define immediate(mn)   (sprintf(buffer, "%s #$%.2x", mn, data[(*offset)++]))
	#define zeropage(mn)    (sprintf(buffer, "%s $%.2x", mn, data[(*offset)++]))
	#define zeropagex(mn)   (sprintf(buffer, "%s $%.2x,X", mn, data[(*offset)++]))
	#define zeropagey(mn)   (sprintf(buffer, "%s $%.2x,Y", mn, data[(*offset)++]))
	#define indirectx(mn)   (sprintf(buffer, "%s ($%.2x,X)", mn, data[(*offset)++]))
	#define indirecty(mn)   (sprintf(buffer, "%s ($%.2x),Y", mn, data[(*offset)++]))
	#define absolute(mn)    (sprintf(buffer, "%s $%.4x", mn, b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define absolutex(mn)   (sprintf(buffer, "%s $%.4x,X", mn, b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define absolutey(mn)   (sprintf(buffer, "%s $%.4x,Y", mn, b16(data[*offset + 1], data[*offset])), *offset += 2)
	#define accumulator(mn) (sprintf(buffer, "%s A", mn))
	#define branch(mn)      (branch_op(mn, data, *offset, labels, labels_idx, buffer), *offset += 1)

	if (labels != NULL) {
		buffer += sprintf(buffer, "B%"PRIdFAST32"_%.4lx: ", *offset / 0x4000, *offset);
		if (*labels_idx >= 128) {
			for (int i = 0; i < 127; ++i)
				labels[i] = labels[i + 1];
			*labels_idx = 127;
		}
		labels[(*labels_idx)++] = *offset;
	}

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
	case 0x90: branch("BCC"); break;
	case 0xB0: branch("BCS"); break;
	case 0xF0: branch("BEQ"); break;
	case 0x30: branch("BMI"); break;
	case 0xD0: branch("BNE"); break;
	case 0x10: branch("BPL"); break;
	case 0x50: branch("BVC"); break;
	case 0x70: branch("BVS"); break;

	// INC
	case 0xE6: zeropage("INC");  break;
	case 0xF6: zeropagex("INC"); break;
	case 0xEE: absolute("INC");  break;
	case 0xFE: absolutex("INC"); break;

	// DEC
	case 0xC6: zeropage("DEC");  break; 
	case 0xD6: zeropagex("DEC"); break;
	case 0xCE: absolute("DEC");  break;
	case 0xDE: absolutex("DEC"); break;

	// LDA
	case 0xA9: immediate("LDA"); break;
	case 0xA5: zeropage("LDA");  break;
	case 0xB5: zeropagex("LDA"); break;
	case 0xAD: absolute("LDA");  break;
	case 0xBD: absolutex("LDA"); break;
	case 0xB9: absolutey("LDA"); break;
	case 0xA1: indirectx("LDA"); break;
	case 0xB1: indirecty("LDA"); break;
	
	// LDX
	case 0xA2: immediate("LDX"); break;
	case 0xA6: zeropage("LDX");  break;
	case 0xB6: zeropagey("LDX"); break;
	case 0xAE: absolute("LDX");  break;
	case 0xBE: absolutey("LDX"); break;

	// LDY
	case 0xA0: immediate("LDY"); break;
	case 0xA4: zeropage("LDY");  break;
	case 0xB4: zeropagex("LDY"); break;
	case 0xAC: absolute("LDY");  break;
	case 0xBC: absolutex("LDY"); break;

	// STA
	case 0x85: zeropage("STA");   break;
	case 0x95: zeropagex("STA");  break;
	case 0x8D: absolute("STA");   break;
	case 0x9D: absolutex("STA");  break;
	case 0x99: absolutey("STA");  break;
	case 0x81: indirectx("STA");  break;
	case 0x91: indirecty("STA");  break;

	// STX
	case 0x86: zeropage("STX");  break;
	case 0x96: zeropagey("STX"); break;
	case 0x8E: absolute("STX");  break;

	// STY
	case 0x84: zeropage("STY");  break;
	case 0x94: zeropagex("STY"); break;
	case 0x8C: absolute("STY");  break;

	// CMP
	case 0xC9: immediate("CMP"); break;
	case 0xC5: zeropage("CMP");  break;
	case 0xD5: zeropagex("CMP"); break;
	case 0xCD: absolute("CMP");  break;
	case 0xDD: absolutex("CMP"); break;
	case 0xD9: absolutey("CMP"); break;
	case 0xC1: indirectx("CMP"); break;
	case 0xD1: indirecty("CMP"); break;

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

	default: sprintf(buffer, ".db $%x", data[*offset - 1]); break;
	}
}


