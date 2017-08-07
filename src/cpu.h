#ifndef UNES_CPU_H_
#define UNES_CPU_H_
#include <stdint.h>
#include <stdbool.h>


#define CPU_FREQ (1789773)

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

enum IrqSource {
	IRQ_SRC_APU_FRAME_COUNTER,
	IRQ_SRC_APU_DMC_TIMER,

	IRQ_SRC_SIZE
};


extern void resetcpu(void);
extern void request_nmi(void);
extern int_fast16_t stepcpu(void);


static inline void set_irq_source(const enum IrqSource src, const bool value)
{
	extern bool cpu_irq_sources[IRQ_SRC_SIZE];
	cpu_irq_sources[src] = value;
}

static inline bool get_irq_source(const enum IrqSource src)
{
	extern bool cpu_irq_sources[IRQ_SRC_SIZE];
	return cpu_irq_sources[src];
}

static inline void trigger_nmi(void)
{
	extern bool cpu_nmi;
	cpu_nmi = true;
}


#endif
