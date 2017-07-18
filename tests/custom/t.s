.include "startup.s"

.segment "CODE"

entry:
	lda #$00
	cmp #$f0
	lda #$f0
	ldx #$00
	cmp #$00
	jmp entry
