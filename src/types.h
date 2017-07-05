#ifndef UNES_TYPES_H_
#define UNES_TYPES_H_


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
