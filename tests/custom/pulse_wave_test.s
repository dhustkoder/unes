.include "startup.s"
.include "apu.s"

.segment "CODE"

_entry:
	jsr init_apu

	lda $17
	sta $4002

	lda $01
	sta $4003

	lda #%10111111
	sta $4000
	
@loop:
	jmp @loop



