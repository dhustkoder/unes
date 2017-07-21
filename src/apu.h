#ifndef UNES_APU_H_
#define UNES_APU_H_
#include <stdint.h>

extern void stepapu(void);
extern void resetapu(void);

extern void apuwrite(uint_fast8_t val, int_fast32_t addr);
extern uint_fast8_t apuread(int_fast32_t addr);


#endif
