ASM=ca65
LD=ld65


all: adc_test.nes pulse_wave_test.nes

%.nes: %.o
	$(LD)  $< -o $@ -C nes.cfg

%.o: %.s
	$(ASM) $< -o $@
