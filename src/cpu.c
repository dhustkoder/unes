#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include "mmu.h"
#include "cpu.h"


#define FLAG_C  (0x01)
#define FLAG_Z  (0x02)
#define FLAG_I  (0x04)
#define FLAG_D  (0x08)
#define FLAG_B  (0x10)
#define FLAG_U  (0x20)
#define FLAG_V  (0x40)
#define FLAG_N  (0x80) 
#define SET_FLAG(flag)       (p |= (flag))
#define CLEAR_FLAG(flag)     (p &= ~(flag))
#define IS_FLAG_SET(flag)    ((p&(flag)) ? true : false)
#define ASSIGN_FLAG(flag, n) ((n) ? SET_FLAG(flag) : CLEAR_FLAG(flag))


static int_fast32_t clk;
static int_fast32_t pc;
static int_fast16_t a, x, y, s, p;


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

static inline void ld(int_fast16_t* const reg, const int_fast16_t val)
{
	*reg = val;
	ASSIGN_FLAG(FLAG_Z, *reg == 0x00);
	ASSIGN_FLAG(FLAG_N, ((*reg)&0x80) == 0x80);
}

static inline void inc(int_fast16_t* const val)
{
	++(*val);
	*val &= 0xFF;
	ASSIGN_FLAG(FLAG_Z, *val == 0);
	ASSIGN_FLAG(FLAG_N, ((*val)&0x80) == 0x80);
}

static inline void dec(int_fast16_t* const val)
{
	--(*val);
	*val &= 0xFF;
	ASSIGN_FLAG(FLAG_Z, *val == 0);
	ASSIGN_FLAG(FLAG_N, ((*val)&0x80) == 0x80);
}

static inline void cmp(const int_fast16_t val)
{
	ASSIGN_FLAG(FLAG_C, a >= val);
	ASSIGN_FLAG(FLAG_Z, a == val);
	ASSIGN_FLAG(FLAG_N, (a&0x80) == 0x80);	
}

static inline void branch(const uint_fast8_t flag, const bool eq)
{
	if (IS_FLAG_SET(flag) == eq) {
		const int_fast8_t val = mmuread(pc++);
		pc += val;
	} else {
		++pc;
	}
}

static inline void incm(const int_fast32_t addr)
{
	int_fast16_t val = mmuread(addr);
	inc(&val);
	mmuwrite(val, addr);
}

static inline void decm(const int_fast32_t addr)
{
	int_fast16_t val = mmuread(addr);
	dec(&val);
	mmuwrite(val, addr);
}

static inline void st(const int_fast16_t* const reg, const int_fast32_t addr)
{
	mmuwrite(*reg, addr);
}

static inline void lda(const uint_fast8_t val) { ld(&a, val); }
static inline void ldx(const uint_fast8_t val) { ld(&x, val); }
static inline void ldy(const uint_fast8_t val) { ld(&y, val); }

static inline void sta(const int_fast32_t addr) { st(&a, addr); }
static inline void stx(const int_fast32_t addr) { st(&x, addr); }
static inline void sty(const int_fast32_t addr) { st(&y, addr); }


static void adc(const int_fast16_t value)
{
	const int carry = IS_FLAG_SET(FLAG_C);
	const bool overflow = a >= 0 && value >= 0 && ((int8_t)(a + value + carry)) < 0;
	ASSIGN_FLAG(FLAG_V, overflow);

	a += value + carry;

	ASSIGN_FLAG(FLAG_C, a > 0xFF);

	a &= 0xFF;

	ASSIGN_FLAG(FLAG_Z, a == 0x00);
	ASSIGN_FLAG(FLAG_N, (a&0x80) == 0x80);
}


void initcpu(void)
{
	pc = mmuread16(ADDR_RESET_VECTOR);
	a = 0x0000;
	x = 0x0000;
	y = 0x0000;
	s = 0x01FA;
	p = 0x0000;
	SET_FLAG(FLAG_I);
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
	#define wabsolutex() (fetch16() + x)
	#define wabsolutey() (fetch16() + y)
	#define windirect()  (fetch16())
	#define windirectx() (mmuread16(((fetch8() + x)&0xFF)))
	#define windirecty() (mmuread16(fetch8()) + y)

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

	const uint_fast8_t opcode = fetch8();

	switch (opcode) {
	// ADC
	case 0x69: adc(immediate());  break; // immediate
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
	*/

	// branches
	case 0x90: branch(FLAG_C, false); break; // BCC
	case 0xB0: branch(FLAG_C, true);  break; // BCS
	case 0xD0: branch(FLAG_Z, false); break; // BNE
	case 0xF0: branch(FLAG_Z, true);  break; // BEQ
	case 0x50: branch(FLAG_V, false); break; // BVC
	case 0x70: branch(FLAG_V, true);  break; // BVS
	case 0x10: branch(FLAG_N, false); break; // BPL
	case 0x30: branch(FLAG_N, true);  break; // BMI

	/*
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
	*/

	// CMP
	case 0xC9: cmp(immediate());  break;
	case 0xC5: cmp(rzeropage());  break;
	case 0xD5: cmp(rzeropagex()); break;
	case 0xCD: cmp(rabsolute());  break;
	case 0xDD: cmp(rabsolutex()); break;
	case 0xD9: cmp(rabsolutey()); break;
	case 0xC1: cmp(rindirectx()); break;
	case 0xD1: cmp(rindirecty()); break;
	/*
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
	*/

	// JSR
	case 0x20:
		spush16(pc + 1);
		pc = wabsolute();
		break;

	// JMP
	case 0x4C: pc = wabsolute(); break;
	case 0x6C: pc = rindirect(); break;


	// implieds
	case 0x00: ASSIGN_FLAG(FLAG_B, !IS_FLAG_SET(FLAG_I));   break; // BRK
	case 0x18: CLEAR_FLAG(FLAG_C);                          break; // CLC
	case 0x38: SET_FLAG(FLAG_C);                            break; // SEC
	case 0x58: CLEAR_FLAG(FLAG_I);                          break; // CLI
	case 0x78: SET_FLAG(FLAG_I);                            break; // SEI
	case 0xB8: CLEAR_FLAG(FLAG_V);                          break; // CLV
	case 0xD8: CLEAR_FLAG(FLAG_D);                          break; // CLD
	case 0xF8: SET_FLAG(FLAG_D);                            break; // SED
	case 0xCA: dec(&x);                                     break; // DEX
	case 0x88: dec(&y);                                     break; // DEY
	case 0xE8: inc(&x);                                     break; // INX
	case 0xC8: inc(&y);                                     break; // INY
	case 0x08: spush(p|0x30);                               break; // PHP
	case 0x28: p = spop();                                  break; // PLP
	case 0x48: spush(a);                                    break; // PHA
	case 0x68: lda(spop());                                 break; // PLA
	case 0xEA:                                              break; // NOP
	case 0x40: p = spop(); pc = spop16();                   break; // RTI
	case 0x60: pc = spop16() + 1;                           break; // RTS
	case 0xAA: ldx(a);                                      break; // TAX
	case 0x8A: lda(x);                                      break; // TXA
	case 0xA8: ldy(a);                                      break; // TAY
	case 0xBA: ldx((s&0xFF));                               break; // TSX
	case 0x9A: s = 0x100 + x;                               break; // TXS
	case 0x98: lda(y);                                      break; // TYA
	default: fprintf(stderr, "UNKOWN OPCODE: %.2x\n", opcode); break;
	}
}

