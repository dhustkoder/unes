#ifndef UNES_CPU_H_
#define UNES_CPU_H_


#define CPU_FREQ (1789773)


extern void resetcpu(void);
extern void request_irq(void);
extern void request_nmi(void);
extern void stepcpu(void);


#endif
