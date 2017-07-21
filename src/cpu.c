#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include "mmu.h"
#include "cpu.h"


static const uint8_t cycles[0x100] = {
//	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*0*/	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
/*1*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*2*/	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
/*3*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*4*/	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
/*5*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*6*/	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
/*7*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*8*/	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/*9*/	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
/*A*/	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/*B*/	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
/*C*/	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/*D*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*E*/	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/*F*/	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};

int_fast32_t clk;
static int_fast32_t pc;
static int_fast16_t a, x, y, s;

static struct {
	uint_fast8_t c, z, i, d, b, v, n;
} flags;


static uint_fast8_t getflags(void)
{
	return (flags.n<<7)|(flags.v<<6)|0x20|(flags.b<<4)|
	(flags.d<<3)|(flags.i<<2)|(flags.z<<1)|(flags.c);
}

static void setflags(const uint_fast8_t val)
{
	flags.n = (val&0x80)>>7;
	flags.v = (val&0x40)>>6;
	flags.b = (val&0x10)>>4;
	flags.d = (val&0x08)>>3;
	flags.i = (val&0x04)>>2;
	flags.z = (val&0x02)>>1;
	flags.c = val&0x01;
}

static inline void spush(const int_fast16_t val)
{
	mmuwrite(val, s--);
}

static inline void spush16(const int_fast32_t val)
{
	mmuwrite16(val, s - 1);
	s -= 2;
}

static inline int_fast16_t spop(void)
{
	return mmuread(++s);
}

static inline int_fast32_t spop16(void)
{
	const int_fast32_t dw = mmuread16(s + 1);
	s += 2;
	return dw;
}

static void ld(int_fast16_t* const reg, const int_fast16_t val)
{
	*reg = val;
	flags.z = *reg == 0x00;
	flags.n = (*reg)>>7;
}

static void inc(int_fast16_t* const val)
{
	++(*val);
	*val &= 0xFF;
	flags.z = *val == 0x00;
	flags.n = (*val)>>7;
}

static void dec(int_fast16_t* const val)
{
	--(*val);
	*val &= 0xFF;
	flags.z = *val == 0x00;
	flags.n = (*val)>>7;
}

static void and(const int_fast16_t val)
{
	a &= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void ora(const int_fast16_t val)
{
	a |= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void eor(const int_fast16_t val)
{
	a ^= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void lsr(int_fast16_t* const val)
{
	flags.c = ((*val)&0x01);
	*val >>= 1;
	flags.z = *val == 0x00;
	flags.n = 0;
}

static void rol(int_fast16_t* const val)
{
	*val = ((*val)<<1)|flags.c;
	flags.c = (*val)>>8;
	*val &= 0xFF;
	flags.z = *val == 0x00;
	flags.n = (*val)>>7;
}

static void ror(int_fast16_t* const val)
{
	const uint_fast8_t oldc = flags.c;
	flags.c = (*val)&0x01;
	(*val) = ((*val)>>1)|(oldc<<7);
	flags.z = *val == 0x00;
	flags.n = *(val)>>7;
}

static void asl(int_fast16_t* const val)
{
	flags.c = (*val)>>7;
	*val <<= 1;
	*val &= 0xFF;
	flags.z = *val == 0x00;
	flags.n = (*val)>>7;
}

static void bit(const int_fast16_t val)
{
	flags.z = (a&val) == 0x00;
	flags.v = (val&0x40)>>6;
	flags.n = val>>7;
}

static void cmp(const int_fast16_t reg, const int_fast16_t val)
{
	flags.c = reg >= val;
	flags.z = reg == val;
	flags.n = ((reg - val)&0x80)>>7;
}

static void branch(const uint_fast8_t flag, const bool eq)
{
	if (flag == eq) {
		const int_fast8_t val = mmuread(pc++);
		pc += val;
	} else {
		++pc;
	}
}

static void adc(const int_fast16_t val)
{
	const int_fast16_t tmp = a + val + flags.c;
	flags.v = (~(a ^ val) & (a ^ tmp))>>7;
	flags.c = tmp>>8;
	a = tmp&0xFF;
	flags.z = a == 0;
	flags.n = a>>7;
}

static inline void sbc(const int_fast16_t val)
{
	adc(val ^ 0xFF);
}

static inline void opm(void(*const op)(int_fast16_t*), const int_fast32_t addr)
{
	int_fast16_t val = mmuread(addr);
	op(&val);
	mmuwrite(val, addr);
}

static inline int_fast32_t chkpagecross(const int_fast32_t addr, const int_fast16_t reg)
{
	// check for page cross in adding register to addr
	// add 1 to clk if it does cross a page
	if (((addr&0xFF) + reg) > 0xFF)
		++clk;
	return addr + reg;
}


void resetcpu(void)
{
	pc = mmuread16(ADDR_RESET_VECTOR);
	a = 0x0000;
	x = 0x0000;
	y = 0x0000;
	s = 0x01FA;
	memset(&flags, 0x00, sizeof(flags));
	flags.i = 1;
}

void stepcpu(void)
{
	#define fetch8()     (mmuread(pc++))
	#define fetch16()    (pc += 2, mmuread16(pc - 2))

	#define immediate()  (fetch8())
	#define wzeropage()  (fetch8())
	#define wzeropagex() ((fetch8() + x)&0xFF)
	#define wzeropagey() ((fetch8() + y)&0xFF)
	#define wabsolute()  (fetch16())
	#define wabsolutex() (chkpagecross(fetch16(), x))
	#define wabsolutey() (chkpagecross(fetch16(), y))
	#define windirect()  (fetch16())
	#define windirectx() (mmuread16(((fetch8() + x)&0xFF)))
	#define windirecty() (chkpagecross(mmuread16(fetch8()), y))

	#define rzeropage()  (mmuread(wzeropage()))
	#define rzeropagex() (mmuread(wzeropagex()))
	#define rzeropagey() (mmuread(wzeropagey()))
	#define rabsolute()  (mmuread(wabsolute()))
	#define rabsolutex() (mmuread(wabsolutex()))
	#define rabsolutey() (mmuread(wabsolutey()))
	#define rindirect()  (mmuread16(windirect()))
	#define rindirectx() (mmuread(windirectx()))
	#define rindirecty() (mmuread(windirecty()))


	assert(pc <= 0xFFFF && a <= 0xFF && x <= 0xFF && 
	       y <= 0xFF && s >= 0x100 && s <= 0x1FF);

	assert((flags.c == 0 || flags.c == 1) &&
	       (flags.z == 0 || flags.z == 1) &&
	       (flags.i == 0 || flags.i == 1) &&
	       (flags.d == 0 || flags.d == 1) &&
	       (flags.b == 0 || flags.b == 1) &&
	       (flags.v == 0 || flags.v == 1) &&
	       (flags.n == 0 || flags.n == 1));

	const uint_fast8_t opcode = fetch8();
	clk += cycles[opcode];

	switch (opcode) {
	// ADC
	case 0x69: adc(immediate());  break;
	case 0x65: adc(rzeropage());  break;
	case 0x75: adc(rzeropagex()); break;
	case 0x6D: adc(rabsolute());  break;
	case 0x7D: adc(rabsolutex()); break;
	case 0x79: adc(rabsolutey()); break;
	case 0x61: adc(rindirectx()); break;
	case 0x71: adc(rindirecty()); break;

	// SBC
	case 0xE9: sbc(immediate());  break;
	case 0xE5: sbc(rzeropage());  break;
	case 0xF5: sbc(rzeropagex()); break;
	case 0xED: sbc(rabsolute());  break;
	case 0xFD: sbc(rabsolutex()); break;
	case 0xF9: sbc(rabsolutey()); break;
	case 0xE1: sbc(rindirectx()); break;
	case 0xF1: sbc(rindirecty()); break;

	// AND
	case 0x29: and(immediate());  break;
	case 0x25: and(rzeropage());  break;
	case 0x35: and(rzeropagex()); break;
	case 0x2D: and(rabsolute());  break;
	case 0x3D: and(rabsolutex()); break;
	case 0x39: and(rabsolutey()); break;
	case 0x21: and(rindirectx()); break;
	case 0x31: and(rindirecty()); break;

	// ORA (OR)
	case 0x09: ora(immediate());  break;
	case 0x05: ora(rzeropage());  break;
	case 0x15: ora(rzeropagex()); break;
	case 0x0D: ora(rabsolute());  break;
	case 0x1D: ora(rabsolutex()); break;
	case 0x19: ora(rabsolutey()); break;
	case 0x01: ora(rindirectx()); break;
	case 0x11: ora(rindirecty()); break;

	// EOR (XOR)
	case 0x49: eor(immediate());  break;
	case 0x45: eor(rzeropage());  break;
	case 0x55: eor(rzeropagex()); break;
	case 0x4D: eor(rabsolute());  break;
	case 0x5D: eor(rabsolutex()); break;
	case 0x59: eor(rabsolutey()); break;
	case 0x41: eor(rindirectx()); break;
	case 0x51: eor(rindirecty()); break;

	// ASL
	case 0x0A: asl(&a);                break;
	case 0x06: opm(asl, wzeropage());  break;
	case 0x16: opm(asl, wzeropagex()); break;
	case 0x0E: opm(asl, wabsolute());  break;
	case 0x1E: opm(asl, wabsolutex()); break;

	// LSR
	case 0x4A: lsr(&a);                 break;
	case 0x46: opm(lsr, wzeropage());   break;
	case 0x56: opm(lsr, wzeropagex());  break;
	case 0x4E: opm(lsr, wabsolute());   break;
	case 0x5E: opm(lsr, wabsolutex());  break;

	// ROL
	case 0x2A: rol(&a);                  break;
	case 0x26: opm(rol, wzeropage());    break;
	case 0x36: opm(rol, wzeropagex());   break;
	case 0x2E: opm(rol, wabsolute());    break;
	case 0x3E: opm(rol, wabsolutex());   break;

	// ROR
	case 0x6A: ror(&a);                  break;
	case 0x66: opm(ror, wzeropage());    break;
	case 0x76: opm(ror, wzeropagex());   break;
	case 0x6E: opm(ror, wabsolute());    break;
	case 0x7E: opm(ror, wabsolutex());   break;

	// BIT
	case 0x24: bit(rzeropage()); break;
	case 0x2C: bit(rabsolute()); break;

	// INC
	case 0xE6: opm(inc, wzeropage());  break;
	case 0xF6: opm(inc, wzeropagex()); break;
	case 0xEE: opm(inc, wabsolute());  break;
	case 0xFE: opm(inc, wabsolutex()); break;

	// DEC
	case 0xC6: opm(dec, wzeropage());  break; 
	case 0xD6: opm(dec, wzeropagex()); break;
	case 0xCE: opm(dec, wabsolute());  break;
	case 0xDE: opm(dec, wabsolutex()); break;

	// branches
	case 0x90: branch(flags.c, false); break; // BCC
	case 0xB0: branch(flags.c, true);  break; // BCS
	case 0xD0: branch(flags.z, false); break; // BNE
	case 0xF0: branch(flags.z, true);  break; // BEQ
	case 0x50: branch(flags.v, false); break; // BVC
	case 0x70: branch(flags.v, true);  break; // BVS
	case 0x10: branch(flags.n, false); break; // BPL
	case 0x30: branch(flags.n, true);  break; // BMI

	// LDA
	case 0xA9: ld(&a, immediate());  break;
	case 0xA5: ld(&a, rzeropage());  break;
	case 0xB5: ld(&a, rzeropagex()); break;
	case 0xAD: ld(&a, rabsolute());  break;
	case 0xBD: ld(&a, rabsolutex()); break;
	case 0xB9: ld(&a, rabsolutey()); break;
	case 0xA1: ld(&a, rindirectx()); break;
	case 0xB1: ld(&a, rindirecty()); break;

	// LDX
	case 0xA2: ld(&x, immediate());  break;
	case 0xA6: ld(&x, rzeropage());  break;
	case 0xB6: ld(&x, rzeropagey()); break;
	case 0xAE: ld(&x, rabsolute());  break;
	case 0xBE: ld(&x, rabsolutey()); break;

	// LDY
	case 0xA0: ld(&y, immediate());  break;
	case 0xA4: ld(&y, rzeropage());  break;
	case 0xB4: ld(&y, rzeropagex()); break;
	case 0xAC: ld(&y, rabsolute());  break;
	case 0xBC: ld(&y, rabsolutex()); break;

	// STA
	case 0x85: mmuwrite(a, wzeropage());  break;
	case 0x95: mmuwrite(a, wzeropagex()); break;
	case 0x8D: mmuwrite(a, wabsolute());  break;
	case 0x9D: mmuwrite(a, wabsolutex()); break;
	case 0x99: mmuwrite(a, wabsolutey()); break;
	case 0x81: mmuwrite(a, windirectx()); break;
	case 0x91: mmuwrite(a, windirecty()); break;

	// STX
	case 0x86: mmuwrite(x, wzeropage());  break;
	case 0x96: mmuwrite(x, wzeropagey()); break;
	case 0x8E: mmuwrite(x, wabsolute());  break;

	// STY
	case 0x84: mmuwrite(y, wzeropage());  break;
	case 0x94: mmuwrite(y, wzeropagex()); break;
	case 0x8C: mmuwrite(y, wabsolute());  break;

	// CMP
	case 0xC9: cmp(a, immediate());  break;
	case 0xC5: cmp(a, rzeropage());  break;
	case 0xD5: cmp(a, rzeropagex()); break;
	case 0xCD: cmp(a, rabsolute());  break;
	case 0xDD: cmp(a, rabsolutex()); break;
	case 0xD9: cmp(a, rabsolutey()); break;
	case 0xC1: cmp(a, rindirectx()); break;
	case 0xD1: cmp(a, rindirecty()); break;

	// CPX
	case 0xE0: cmp(x, immediate());  break;
	case 0xE4: cmp(x, rzeropage());  break;
	case 0xEC: cmp(x, rabsolute());  break;

	// CPY
	case 0xC0: cmp(y, immediate());  break;
	case 0xC4: cmp(y, rzeropage());  break;
	case 0xCC: cmp(y, rabsolute());  break;

	// JSR
	case 0x20:
		spush16(pc + 1);
		pc = wabsolute();
		break;

	// JMP
	case 0x4C: pc = wabsolute(); break;
	case 0x6C: pc = rindirect(); break;

	// implieds
	case 0x00: flags.b = !flags.i;                          break; // BRK
	case 0x18: flags.c = 0;                                 break; // CLC
	case 0x38: flags.c = 1;                                 break; // SEC
	case 0x58: flags.i = 0;                                 break; // CLI
	case 0x78: flags.i = 1;                                 break; // SEI
	case 0xB8: flags.v = 0;                                 break; // CLV
	case 0xD8: flags.d = 0;                                 break; // CLD
	case 0xF8: flags.d = 1;                                 break; // SED
	case 0xCA: dec(&x);                                     break; // DEX
	case 0x88: dec(&y);                                     break; // DEY
	case 0xE8: inc(&x);                                     break; // INX
	case 0xC8: inc(&y);                                     break; // INY
	case 0x08: spush(getflags());                           break; // PHP
	case 0x28: setflags(spop());                            break; // PLP
	case 0x48: spush(a);                                    break; // PHA
	case 0x68: ld(&a, spop());                              break; // PLA
	case 0xEA:                                              break; // NOP
	case 0x40: setflags(spop()); pc = spop16();             break; // RTI
	case 0x60: pc = spop16() + 1;                           break; // RTS
	case 0xAA: ld(&x, a);                                   break; // TAX
	case 0x8A: ld(&a, x);                                   break; // TXA
	case 0xA8: ld(&y, a);                                   break; // TAY
	case 0xBA: ld(&x, (s&0xFF));                            break; // TSX
	case 0x9A: s = 0x100 + x;                               break; // TXS
	case 0x98: ld(&a, y);                                   break; // TYA
	default: fprintf(stderr, "UNKOWN OPCODE: %.2x\n", opcode); break;
	}
}

