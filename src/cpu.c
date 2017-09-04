#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "log.h"
#include "input.h"
#include "rom.h"
#include "ppu.h"
#include "apu.h"
#include "cpu.h"


#define ADDR_NMI_VECTOR   (0xFFFA)
#define ADDR_RESET_VECTOR (0xFFFC)
#define ADDR_IRQ_VECTOR   (0xFFFE)
#define ADDR_PRGROM_UPPER (0xC000)
#define ADDR_PRGROM       (0x8000)
#define ADDR_SRAM         (0x6000)
#define ADDR_EXPROM       (0x4020)
#define ADDR_IOREGS2      (0x4000)
#define ADDR_MIRRORS2     (0x2008)
#define ADDR_IOREGS1      (0x2000)
#define ADDR_MIRRORS1     (0x0800)
#define ADDR_RAM          (0x0200)
#define ADDR_STACK        (0x0100)
#define ADDR_ZEROPAGE     (0x0000)

#define FLAG_C (0x01)
#define FLAG_Z (0x02)
#define FLAG_I (0x04)
#define FLAG_D (0x08)
#define FLAG_B (0x10) // does not physicaly exist but is set or clear in some operations
#define FLAG_R (0x20) // not used but always set
#define FLAG_V (0x40)
#define FLAG_N (0x80)


// cpu.c globals
bool cpu_nmi;
bool cpu_irq_sources[IRQ_SRC_SIZE];
const uint8_t* cpu_prgrom[2]; // lower and upper banks, switching is done in rom.c 

// cpu.c
static uint16_t step_cycles;
static bool irq_pass;
static uint16_t pc;
static uint8_t a, x, y, s;
static struct { bool c : 1, z : 1, i : 1, d : 1, v : 1, n : 1; } flags;
static uint8_t padstate[2];
static int8_t padshifts[2];
static bool padstrobe;
static uint8_t ram[0x800];  // zeropage,stack,ram
static uint8_t sram[0x2000];

static uint8_t getflags(void)
{
	return (flags.n<<7)|(flags.v<<6)|FLAG_R|(flags.d<<3)|
	       (flags.i<<2)|(flags.z<<1)|(flags.c);
}

static void setflags(const uint8_t val)
{
	flags.n = (val>>7)&0x01;
	flags.v = (val>>6)&0x01;
	flags.d = (val>>3)&0x01;
	flags.i = (val>>2)&0x01;
	flags.z = (val>>1)&0x01;
	flags.c = val&0x01;
}


// cpu memory bus
static void oam_dma(uint8_t val);

static void joywrite(const uint8_t val)
{
	const bool oldstrobe = padstrobe;
	padstrobe = (val&0x01) != 0;
	if (oldstrobe && !padstrobe) {
		for (unsigned pad = 0; pad < 2; ++pad) {
			padshifts[pad] = 0;
			padstate[pad] = getpadstate(pad);
		}
	}
}

static uint8_t joyread(const uint16_t addr)
{
	assert(addr == 0x4016 || addr == 0x4017);

	const unsigned pad = addr == 0x4016 ? JOYPAD_ONE : JOYPAD_TWO;

	if (padstrobe)
		return getpadstate(pad)&0x01;
	else if (padshifts[pad] >= 8)
		return 0x01;
	
	const uint8_t k = padstate[pad]&0x01;
	padstate[pad] >>= 1;
	++padshifts[pad];
	return k;
}

static uint8_t ioread(const uint16_t addr)
{
	if (addr >= 0x4016)
		return joyread(addr);
	else if (addr == 0x4015)
		return apuread_status();
	else
		return ppuread(addr);
}

static void iowrite(const uint8_t val, const uint16_t addr)
{
	if (addr >= 0x4000) {
		switch (addr) {
		default: apuwrite(val, addr); break;
		case 0x4014: oam_dma(val);    break;
		case 0x4016: joywrite(val);   break;
		}
	} else {
		ppuwrite(val, addr);
	}
}

static uint8_t read(const uint16_t addr)
{
	if (addr >= ADDR_PRGROM)
		return cpu_prgrom[(addr>>14)&0x01][addr&0x3FFF];
	else if (addr < ADDR_IOREGS1)
		return ram[addr&0x7FF];
	else if (addr < ADDR_EXPROM)
		return ioread(addr);
	else if (addr >= ADDR_SRAM)
		return sram[addr&0x1FFF];

	return 0;
}

static void write(const uint8_t val, const uint16_t addr)
{
	if (addr < ADDR_IOREGS1)
		ram[addr&0x7FF] = val;
	else if (addr < ADDR_EXPROM)
		iowrite(val, addr);
	else if (addr >= ADDR_PRGROM)
		romwrite(val, addr);
	else if (addr >= ADDR_SRAM)
		sram[addr&0x1FFF] = val;
}

static void oam_dma(const uint8_t val)
{
	extern uint8_t ppu_oam[0x100];
	extern bool ppu_need_screen_update;
	
	const unsigned offset = 0x100 * val;
	if (offset < 0x2000 && (offset&0x1FFF) <= 0x1F00) {
		const uint8_t* const pram = &ram[offset&0x7FF];
		if (memcmp(ppu_oam, pram, 0x100) != 0) {
			memcpy(ppu_oam, pram, 0x100);
			ppu_need_screen_update = true;
		}
	} else {
		for (unsigned i = 0; i < 0x100; ++i)
			ppu_oam[i] = read(offset + i);
		ppu_need_screen_update = true;
	}
	step_cycles += 513;
}

static uint16_t read16(const uint16_t addr)
{
	return (read(addr + 1)<<8)|read(addr);
}

static uint16_t read16msk(const uint16_t addr)
{
	if ((addr&0x00FF) == 0xFF)
		return (read(addr&0xFF00)<<8)|read(addr);
	return read16(addr);
}


// stack
static void spush(const uint8_t val)
{
	ram[0x100|s--] = val;
}

static void spush16(const uint16_t val)
{
	spush((val&0xFF00)>>8);
	spush(val&0xFF);
}

static uint8_t spop(void)
{
	return ram[0x100|++s];
}

static uint16_t spop16(void)
{
	const uint8_t lsb = spop();
	const uint16_t msb = spop();
	return (msb<<8)|lsb;
}


// instructions
static inline void ld(uint8_t* const reg, const uint8_t val)
{
	*reg = val;
	flags.z = *reg == 0x00;
	flags.n = (*reg)>>7;
}

static uint8_t inc(uint8_t val)
{
	++val;
	flags.z = val == 0x00;
	flags.n = val>>7;
	return val;
}

static uint8_t dec(uint8_t val)
{
	--val;
	flags.z = val == 0x00;
	flags.n = val>>7;
	return val;
}

static uint8_t lsr(uint8_t val)
{
	flags.c = val&0x01;
	val >>= 1;
	flags.z = val == 0x00;
	flags.n = 0;
	return val;
}

static uint8_t rol(uint8_t val)
{
	const uint8_t oldc = flags.c;
	flags.c = val>>7;
	val = (val<<1)|oldc;
	flags.z = val == 0x00;
	flags.n = val>>7;
	return val;
}

static uint8_t ror(uint8_t val)
{
	const uint8_t oldc = flags.c;
	flags.c = val&0x01;
	val = (val>>1)|(oldc<<7);
	flags.z = val == 0x00;
	flags.n = val>>7;
	return val;
}

static uint8_t asl(uint8_t val)
{
	flags.c = val>>7;
	val <<= 1;
	flags.z = val == 0x00;
	flags.n = val>>7;
	return val;
}

static inline void opm(uint8_t(*const op)(uint8_t), const uint16_t addr)
{
	write(op(read(addr)), addr);
}

static inline void opzp(uint8_t(*const op)(uint8_t), const uint8_t addr)
{
	ram[addr] = op(ram[addr]);
}

static void and(const uint8_t val)
{
	a &= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void ora(const uint8_t val)
{
	a |= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void eor(const uint8_t val)
{
	a ^= val;
	flags.z = a == 0x00;
	flags.n = a>>7;
}

static void bit(const uint8_t val)
{
	flags.z = (a&val) == 0x00;
	flags.v = (val&0x40)>>6;
	flags.n = val>>7;
}

static void cmp(const uint8_t reg, const uint8_t val)
{
	flags.c = reg >= val;
	flags.z = reg == val;
	flags.n = ((reg - val)&0x80)>>7;
}

static void adc(const int16_t val)
{
	const unsigned tmp = a + val + flags.c;
	flags.v = (((~(a ^ val) & (a ^ tmp)))&0x80)>>7;
	flags.c = tmp>>8;
	a = tmp&0xFF;
	flags.z = a == 0;
	flags.n = a>>7;
}

static void sbc(const uint8_t val)
{
	adc(val ^ 0xFF);
}

static uint16_t chkpagecross(const uint16_t addr, const int16_t val)
{
	// check for page cross in adding value to addr
	// add 1 to step_cycles if it does cross a page
	if ((addr&0xFF00) != ((addr + val)&0xFF00))
		++step_cycles;
	return (addr + val)&0xFFFF;
}

static void branch(const bool cond)
{
	if (cond) {
		const int8_t val = read(pc++);
		++step_cycles;
		pc = chkpagecross(pc, val);
	} else {
		++pc;
	}
}

static bool check_irq_sources(void)
{
	for (unsigned i = 0; i < IRQ_SRC_SIZE; ++i)
		if (cpu_irq_sources[i])
			return true;
	return false;
}

static void dointerrupt(const uint16_t vector, const bool brk)
{
	spush16(brk ? pc + 1 : pc);
	spush(getflags()|(brk ? FLAG_B : 0x00));
	pc = read16(vector);
	flags.i = 1;
	step_cycles += 7;
}



void resetcpu(void)
{
	cpu_nmi = false;
	memset(cpu_irq_sources, 0, sizeof cpu_irq_sources);
	irq_pass = false;

	pc = read16(ADDR_RESET_VECTOR);
	a = 0x00;
	x = 0x00;
	y = 0x00;
	s = 0xFD;

	memset(&flags, 0, sizeof flags);
	flags.i = true;
	memset(&padstate, 0, sizeof padstate);
	memset(&padshifts, 0, sizeof padshifts);
	padstrobe = false;
}

unsigned stepcpu(void)
{
	#define fetch8()            (read(pc++))
	#define fetch16()           (pc += 2, read16(pc - 2))
	#define writezp(data, addr) (ram[addr] = data)

	#define immediate()      (fetch8())
	#define wzeropage()      (fetch8())
	#define wzeropagex()     ((fetch8() + x)&0xFF)
	#define wzeropagey()     ((fetch8() + y)&0xFF)
	#define wabsolute()      (fetch16())
	#define wabsolutex()     (chkpagecross(fetch16(), x))
	#define wabsolutexnchk() ((fetch16() + x)&0xFFFF)
	#define wabsolutey()     (chkpagecross(fetch16(), y))
	#define wabsoluteynchk() ((fetch16() + y)&0xFFFF)
	#define windirectx()     (read16msk((fetch8() + x)&0xFF))
	#define windirecty()     (chkpagecross(read16msk(fetch8()), y))
	#define windirectynchk() ((read16msk(fetch8()) + y)&0xFFFF)

	#define rzeropage()  (ram[wzeropage()])
	#define rzeropagex() (ram[wzeropagex()])
	#define rzeropagey() (ram[wzeropagey()])
	#define rabsolute()  (read(wabsolute()))
	#define rabsolutex() (read(wabsolutex()))
	#define rabsolutey() (read(wabsolutey()))
	#define rindirectx() (read(windirectx()))
	#define rindirecty() (read(windirecty()))

	static const uint8_t clock_table[0x100] = {
		      /*0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F*/
		/*0*/	0, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
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

	step_cycles = 0;

	if (cpu_nmi) {
		dointerrupt(ADDR_NMI_VECTOR, false);
		cpu_nmi = false;
	} else if (irq_pass && check_irq_sources()) {
		dointerrupt(ADDR_IRQ_VECTOR, false);
	}

	irq_pass = flags.i == 0;
	const uint8_t opcode = fetch8();
	step_cycles += clock_table[opcode];

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
	case 0x0A: a = asl(a);                 break;
	case 0x06: opzp(asl, wzeropage());     break;
	case 0x16: opzp(asl, wzeropagex());    break;
	case 0x0E: opm(asl, wabsolute());      break;
	case 0x1E: opm(asl, wabsolutexnchk()); break;

	// LSR
	case 0x4A: a = lsr(a);                 break;
	case 0x46: opzp(lsr, wzeropage());     break;
	case 0x56: opzp(lsr, wzeropagex());    break;
	case 0x4E: opm(lsr, wabsolute());      break;
	case 0x5E: opm(lsr, wabsolutexnchk()); break;

	// ROL
	case 0x2A: a = rol(a);                 break;
	case 0x26: opzp(rol, wzeropage());     break;
	case 0x36: opzp(rol, wzeropagex());    break;
	case 0x2E: opm(rol, wabsolute());      break;
	case 0x3E: opm(rol, wabsolutexnchk()); break;

	// ROR
	case 0x6A: a = ror(a);                 break;
	case 0x66: opzp(ror, wzeropage());     break;
	case 0x76: opzp(ror, wzeropagex());    break;
	case 0x6E: opm(ror, wabsolute());      break;
	case 0x7E: opm(ror, wabsolutexnchk()); break;

	// BIT
	case 0x24: bit(rzeropage()); break;
	case 0x2C: bit(rabsolute()); break;

	// INC
	case 0xE6: opzp(inc, wzeropage());     break;
	case 0xF6: opzp(inc, wzeropagex());    break;
	case 0xEE: opm(inc, wabsolute());      break;
	case 0xFE: opm(inc, wabsolutexnchk()); break;

	// DEC
	case 0xC6: opzp(dec, wzeropage());     break; 
	case 0xD6: opzp(dec, wzeropagex());    break;
	case 0xCE: opm(dec, wabsolute());      break;
	case 0xDE: opm(dec, wabsolutexnchk()); break;

	// branches
	case 0x90: branch(!flags.c); break; // BCC
	case 0xB0: branch(flags.c);  break; // BCS
	case 0xD0: branch(!flags.z); break; // BNE
	case 0xF0: branch(flags.z);  break; // BEQ
	case 0x50: branch(!flags.v); break; // BVC
	case 0x70: branch(flags.v);  break; // BVS
	case 0x10: branch(!flags.n); break; // BPL
	case 0x30: branch(flags.n);  break; // BMI

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
	case 0x85: writezp(a, wzeropage());    break;
	case 0x95: writezp(a, wzeropagex());   break;
	case 0x8D: write(a, wabsolute());      break;
	case 0x9D: write(a, wabsolutexnchk()); break;
	case 0x99: write(a, wabsoluteynchk()); break;
	case 0x81: write(a, windirectx());     break;
	case 0x91: write(a, windirectynchk()); break;

	// STX
	case 0x86: writezp(x, wzeropage());  break;
	case 0x96: writezp(x, wzeropagey()); break;
	case 0x8E: write(x, wabsolute());    break;

	// STY
	case 0x84: writezp(y, wzeropage());  break;
	case 0x94: writezp(y, wzeropagex()); break;
	case 0x8C: write(y, wabsolute());    break;

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
	case 0x4C: pc = wabsolute();          break;
	case 0x6C: pc = read16msk(fetch16()); break;

	// implieds
	case 0x00: dointerrupt(ADDR_IRQ_VECTOR, true);          break; // BRK
	case 0x18: flags.c = false;                             break; // CLC
	case 0x38: flags.c = true;                              break; // SEC
	case 0x58: flags.i = false;                             break; // CLI
	case 0x78: flags.i = true;                              break; // SEI
	case 0xB8: flags.v = false;                             break; // CLV
	case 0xD8: flags.d = false;                             break; // CLD
	case 0xF8: flags.d = true;                              break; // SED
	case 0xCA: x = dec(x);                                  break; // DEX
	case 0x88: y = dec(y);                                  break; // DEY
	case 0xE8: x = inc(x);                                  break; // INX
	case 0xC8: y = inc(y);                                  break; // INY
	case 0x08: spush(getflags()|FLAG_B);                    break; // PHP
	case 0x28: setflags(spop());                            break; // PLP
	case 0x48: spush(a);                                    break; // PHA
	case 0x68: ld(&a, spop());                              break; // PLA
	case 0xEA:                                              break; // NOP
	case 0x40: // RTI
		setflags(spop());
		pc = spop16();
		irq_pass = flags.i == 0;
		break;
	case 0x60: pc = spop16() + 1;                           break; // RTS
	case 0xAA: ld(&x, a);                                   break; // TAX
	case 0x8A: ld(&a, x);                                   break; // TXA
	case 0xA8: ld(&y, a);                                   break; // TAY
	case 0xBA: ld(&x, (s&0xFF));                            break; // TSX
	case 0x9A: s = x;                                       break; // TXS
	case 0x98: ld(&a, y);                                   break; // TYA
	default:
		logerror("UNKOWN OPCODE: $%.2x\n PC: $%.2x\n", opcode, pc);
		assert(false);
		break;
	}

	return step_cycles;
}
