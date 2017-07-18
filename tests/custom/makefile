ASM=ca65
LD=ld65

all: adc.nes t.nes

adc.nes:
	$(ASM) adc.s -o adc.o
	$(LD)  adc.o -o adc.nes -C nes.cfg

t.nes:
	$(ASM) t.s -o t.o
	$(LD) t.o -o t.nes -C nes.cfg
