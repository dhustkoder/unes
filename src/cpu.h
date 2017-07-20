#ifndef UNES_CPU_H_
#define UNES_CPU_H_


static inline int_fast32_t get_cpu_clock(void)
{
	extern int_fast32_t clk;
	return clk;
}

static inline void set_cpu_clock(const int_fast32_t val)
{
	extern int_fast32_t clk;
	clk = val;
}

extern void resetcpu(void);
extern void stepcpu(void);

#endif
