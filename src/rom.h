#ifndef UNES_ROM_H_
#define UNES_ROM_H_
#include <stdbool.h>
#include <stdint.h>


#define PRGROM_BANK_SIZE (0x4000)
#define CHR_BANK_SIZE    (0x2000)
#define SRAM_BANK_SIZE   (0x2000)
#define TRAINER_SIZE     (0x0200)

typedef uint8_t mapper_type_t;
enum MapperType {
	MAPPER_TYPE_NROM,
	MAPPER_TYPE_MMC1
};

typedef uint8_t nt_mirroring_mode_t;
enum NTMirroringMode {
	NT_MIRRORING_MODE_HORIZONTAL,
	NT_MIRRORING_MODE_VERTICAL,
	NT_MIRRORING_MODE_ONE_SCREEN_LOW,
	NT_MIRRORING_MODE_ONE_SCREEN_UPPER
};


extern bool rom_load(const uint8_t* data);
extern void rom_unload(void);
extern void rom_write(uint8_t value, uint16_t addr);


#endif
