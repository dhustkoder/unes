#ifndef UNES_APU_H_
#define UNES_APU_H_
#include <stdint.h>

extern void apu_reset(void);
extern void apu_step(short aputicks);
extern void apu_write(uint8_t val, uint16_t addr);
extern uint8_t apu_read_status(void);

#endif
