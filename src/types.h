#ifndef UNES_TYPES_H_
#define UNES_TYPES_H_
#include <stdint.h>

#define PRGROM_BANK_SIZE ((int_fast32_t)16384)
#define VROM_BANK_SIZE   ((int_fast32_t)8192)
#define RAM_BANK_SIZE    ((int_fast32_t)8192)
#define TRAINER_SIZE     ((int_fast32_t)512)

typedef struct rom {
	uint8_t prgrom_num_banks;
	uint8_t vrom_num_banks;
	uint8_t ram_num_banks;	
	uint8_t ctrl1;
	uint8_t ctrl2;
	uint8_t data[];
} rom_t;


typedef struct opcode {
	uint8_t code;
	uint8_t bytes;
} opcode_t;


#endif
