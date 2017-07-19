ASM=ca65
LD=ld65

.phony: all clean


all: adc_test.nes pulse_wave_test.nes mirrors_test.nes tmp.nes

%.nes: %.o
	$(LD)  $< -o $@ -C nes.cfg

%.o: %.s
	$(ASM) $< -o $@

clean:
	rm -rf *.o *.deb
