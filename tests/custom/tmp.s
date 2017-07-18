.include "startup.s"

.segment "CODE"

_entry:
	clv
	lda #$7f
	sec
	adc #$00
	jmp _entry

