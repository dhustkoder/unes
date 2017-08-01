#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdbool.h>
#include <stdint.h>


#define PRGROM_BANK_SIZE   ((int_fast32_t)0x4000)
#define CHR_BANK_SIZE      ((int_fast32_t)0x2000)
#define CART_RAM_BANK_SIZE ((int_fast32_t)0x2000)
#define TRAINER_SIZE       ((int_fast32_t)0x0200)


extern bool loadrom(const char* path);
extern void freerom(void);

extern uint_fast8_t romread(uint_fast16_t addr);
extern void romwrite(uint_fast8_t value, uint_fast16_t addr);

#endif
