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
	int_fast32_t offset = 0;
	int_fast32_t labels[128];
	int labels_idx = 0;
	char buffer[32];

	puts("\nPRG-ROM:\n");
	while (offset < rom->prgrom_size) {
		opstr(rom->data, &offset, labels, &labels_idx, buffer);
		puts(buffer);
	}

	if (rom->vrom_num_banks > 0) {
		const uint8_t* const vrom = &rom->data[rom->prgrom_size];
		printf("\n\nVROM:\n\n");
		for (offset = 0; offset < rom->vrom_size; ++offset) {
			if ((offset % 16) == 0)
				putchar('\n');
			else if ((offset % 2) == 0)
				putchar(' ');
			printf("%.2x", vrom[offset]);
		}
	}
}


static void branch_op(const char* const mnemonic,
                      const uint8_t* const data,
                      const int_fast32_t offset,
		      const int_fast32_t labels[],
		      const int* const labels_idx,
		      char buffer[])
{
	if (labels != NULL) {
		const int_fast32_t target = offset + 1 + ((int8_t)data[offset]);
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
		} else {
			sprintf(buffer, "%s B%"PRIdFAST32"_%.4lx", mnemonic, offset / 0x4000, target);
		}
	}
}


static inline void opstr(const uint8_t* const data,
                         int_fast32_t* const offset,
			 int_fast32_t labels[128],
			 int* const labels_idx,
			 char buffer[32])
{
	#define b16(msb, lsb)   (((msb)<<8)|(lsb))
	#define implied(mn)     (sprintf(buffer, "%s", mn))
	#define immediate(mn)   (sprintf(buffer, "%s #$%.2x", mn, data[(*offset)++]))
	#define zeropage(mn)    (sprintf(buffer, "%s $%.2x", mn, data[(*offset)++]))
	#define zeropagex(mn)   (sprintf(buffer, "%s $%.2x,X", mn, data[(*offset)++]))
	#define zeropagey(mn)   (sprintf(buffer, "%s $%.2x,Y", mn, data[(*offset)++]))
	#define indirect(mn)    (sprintf(buffer, "%s ($%.4x)", mn, b16(data[*offset + 1], data[*offset])), *offset += 2)
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

	// branches
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
	case 0x85: zeropage("STA");  break;
	case 0x95: zeropagex("STA"); break;
	case 0x8D: absolute("STA");  break;
	case 0x9D: absolutex("STA"); break;
	case 0x99: absolutey("STA"); break;
	case 0x81: indirectx("STA"); break;
	case 0x91: indirecty("STA"); break;

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
	case 0x6A: accumulator("ROR"); break;
	case 0x66: zeropage("ROR");    break;
	case 0x76: zeropagex("ROR");   break;
	case 0x6E: absolute("ROR");    break;
	case 0x7E: absolutex("ROR");   break;

	// BIT
	case 0x24: zeropage("BIT"); break;
	case 0x2C: absolute("BIT"); break;

	// CPX
	case 0xE0: immediate("CPX"); break;
	case 0xE4: zeropage("CPX");  break;
	case 0xEC: absolute("CPX");  break;

	// CPY
	case 0xC0: immediate("CPY"); break;
	case 0xC4: zeropage("CPY");  break;
	case 0xCC: absolute("CPY");  break;

	// JSR
	case 0x20: absolute("JSR"); break;

	// JMP
	case 0x4C: absolute("JMP"); break;
	case 0x6C: indirect("JMP"); break;

	// implieds
	case 0x00: implied("BRK"); break;
	case 0x18: implied("CLC"); break;
	case 0x58: implied("CLI"); break;
	case 0x78: implied("SEI"); break;
	case 0xB8: implied("CLV"); break;
	case 0xD8: implied("CLD"); break;
	case 0xF8: implied("SED"); break;
	case 0xCA: implied("DEX"); break;
	case 0x88: implied("DEY"); break;
	case 0xE8: implied("INX"); break;
	case 0xC8: implied("INY"); break;
	case 0x08: implied("PHP"); break;
	case 0x28: implied("PLP"); break;
	case 0x48: implied("PHA"); break;
	case 0x68: implied("PLA"); break;
	case 0xEA: implied("NOP"); break;
	case 0x40: implied("RTI"); break;
	case 0x60: implied("RTS"); break;
	case 0x38: implied("SEC"); break;
	case 0xAA: implied("TAX"); break;
	case 0x8A: implied("TXA"); break;
	case 0xA8: implied("TAY"); break;
	case 0xBA: implied("TSX"); break;
	case 0x9A: implied("TXS"); break;
	case 0x98: implied("TYA"); break;
	default: sprintf(buffer, ".db $%x", data[*offset - 1]); break;
	}
}


