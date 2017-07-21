#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdbool.h>
#include <stdint.h>


extern bool loadrom(const char* path);
extern void freerom(void);

extern uint_fast8_t romread(int_fast32_t addr);

#endif
