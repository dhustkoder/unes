#include <stdio.h>
#include "types.h"
#include "mem.h"
#include "cpu.h"


#define FLAG_C  (0x01)
#define FLAG_Z  (0x02)
#define FLAG_I  (0x04)
#define FLAG_D  (0x08)
#define FLAG_V  (0x40)
#define FLAG_N  (0x80) 
#define SET_FLAG(flag)    (p |= (flag))
#define UNSET_FLAG(flag)  (p &= ~(flag))
#define IS_FLAG_SET(flag) ((p&(flag)) ? 1 : 0)


static int_fast32_t clk;
static int_fast32_t pc;
static int_fast32_t a, x, y, s, p;


static void adc(const uint_fast8_t value)
{
	const int carry = IS_FLAG_SET(FLAG_C);

	if (a > 0 && value > 0 && ((int8_t)(a + value + carry)) < 0)
		SET_FLAG(FLAG_V);

	a += value + carry;

	if (a > 0xFF)
		SET_FLAG(FLAG_C);

	a &= 0xFF;

	if (a == 0x00)
		SET_FLAG(FLAG_Z);
	else if ((a&0x80) == 0x80)
		SET_FLAG(FLAG_N);

}

static void ld(int_fast32_t* const reg, const uint_fast8_t val)
{
	*reg = val;
	if (*reg == 0)
		SET_FLAG(FLAG_Z);
	else if ((*reg&0x80) == 0x80)
		SET_FLAG(FLAG_N);
}

static void st(const int_fast32_t* const reg, const uint_fast16_t addr)
{
	memwrite(*reg, addr);
}

static void jmp(const uint_fast16_t addr)
{
	pc = addr;
}

static inline void lda(const uint_fast8_t val) { ld(&a, val); }
static inline void ldx(const uint_fast8_t val) { ld(&x, val); }
static inline void ldy(const uint_fast8_t val) { ld(&y, val); }

static inline void sta(const uint_fast16_t addr) { st(&a, addr); }
static inline void stx(const uint_fast16_t addr) { st(&x, addr); }
static inline void sty(const uint_fast16_t addr) { st(&y, addr); }



void initcpu(void)
{
	pc = memread16(ADDR_RESET_VECTOR);
	a = x = y = s = p = clk = 0;
}


void stepcpu(void)
{
	#define fetch8()     (memread(pc++))
	#define fetch16()    (pc += 2, memread16(pc - 2))

	#define immediate() (fetch8())

	#define wzeropage()  (fetch8())
	#define wzeropagex() ((fetch8() + x)&0xFF)
	#define wzeropagey() ((fetch8() + y)&0xFF)
	#define wabsolute()  (fetch16())
	#define wabsolutex() (fetch16() + x)
	#define wabsolutey() (fetch16() + y)
	#define windirect()  (fetch16())
	#define windirectx() (memread16(((fetch8() + x)&0xFF)))
	#define windirecty() (memread16(fetch8()) + y)

	#define rzeropage()  (memread(wzeropage()))
	#define rzeropagex() (memread(wzeropagex()))
	#define rzeropagey() (memread(wzeropagey()))
	#define rabsolute()  (memread(wabsolute()))
	#define rabsolutex() (memread(wabsolutex()))
	#define rabsolutey() (memread(wabsolutey()))
	#define rindirect()  (memread16(windirect()))
	#define rindirectx() (memread(windirectx()))
	#define rindirecty() (memread(windirecty()))

	const uint_fast8_t opcode = fetch8();

	switch (opcode) {
	// ADC
	case 0x69: adc(immediate()); break;  // immediate
	case 0x65: adc(rzeropage());  break; // zeropage
	case 0x75: adc(rzeropagex()); break; // zeropage x
	case 0x6D: adc(rabsolute());  break; // absolute
	case 0x7D: adc(rabsolutex()); break; // absolute x
	case 0x79: adc(rabsolutey()); break; // absolute y
	case 0x61: adc(rindirectx()); break; // indirect x
	case 0x71: adc(rindirecty()); break; // indirect y

	/*
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
	*/

	// LDA
	case 0xA9: lda(immediate());  break;
	case 0xA5: lda(rzeropage());  break;
	case 0xB5: lda(rzeropagex()); break;
	case 0xAD: lda(rabsolute());  break;
	case 0xBD: lda(rabsolutex()); break;
	case 0xB9: lda(rabsolutey()); break;
	case 0xA1: lda(rindirectx()); break;
	case 0xB1: lda(rindirecty()); break;

	// LDX
	case 0xA2: ldx(immediate());  break;
	case 0xA6: ldx(rzeropage());  break;
	case 0xB6: ldx(rzeropagey()); break;
	case 0xAE: ldx(rabsolute());  break;
	case 0xBE: ldx(rabsolutey()); break;

	// LDY
	case 0xA0: ldy(immediate());  break;
	case 0xA4: ldy(rzeropage());  break;
	case 0xB4: ldy(rzeropagex()); break;
	case 0xAC: ldy(rabsolute());  break;
	case 0xBC: ldy(rabsolutex()); break;

	// STA
	case 0x85: sta(wzeropage());  break;
	case 0x95: sta(wzeropagex()); break;
	case 0x8D: sta(wabsolute());  break;
	case 0x9D: sta(wabsolutex()); break;
	case 0x99: sta(wabsolutey()); break;
	case 0x81: sta(windirectx()); break;
	case 0x91: sta(windirecty()); break;

	/*
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
	*/

	// JMP
	case 0x4C: jmp(wabsolute()); break;
	case 0x6C: jmp(rindirect()); break;

	/*
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
	*/
	default: fprintf(stderr, "UNKOWN OPCODE: %.2x\n", opcode);
	}
}

