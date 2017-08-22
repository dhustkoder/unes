#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdbool.h>
#include <stdint.h>


#define PRGROM_BANK_SIZE (0x4000)
#define CHR_BANK_SIZE    (0x2000)
#define SRAM_BANK_SIZE   (0x2000)
#define TRAINER_SIZE     (0x0200)


enum MapperType {
	NROM,
	MMC1
};

enum NTMirroringMode {
	NTMIRRORING_HORIZONTAL,
	NTMIRRORING_VERTICAL,
	NTMIRRORING_ONE_SCREEN_LOW,
	NTMIRRORING_ONE_SCREEN_UPPER
};


extern bool loadrom(const uint8_t* restrict data);
extern void freerom(void);
extern void romwrite(uint8_t value, uint16_t addr);


#endif
