#ifndef UNES_APU_H_
#define UNES_APU_H_
#include <stdint.h>

extern void resetapu(void);
extern void stepapu(unsigned aputicks);
extern void apuwrite(uint_fast8_t val, uint_fast16_t addr);
extern uint_fast8_t apuread_status(void);

#endif
