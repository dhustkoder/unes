ASM=ca65
LD=ld65


all: adc.nes apu.nes

%.nes: adc.o apu.o
	$(LD)  $< -o $@ -C nes.cfg

%.o: %.s
	$(ASM) $< -o $@
