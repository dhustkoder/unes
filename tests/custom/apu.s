.segment "CODE"

init_apu:
	; Init $4000-4013
	ldy #$13
@loop:
	lda @regs,y
	sta $4000,y
	dey
	bpl @loop
	
	; we have to skip over $4014 (OAMDMA)
	lda #$0f
	sta $4015
	lda #$40
	sta $4017

	rts
@regs:
	.byte $30,$08,$00,$00
	.byte $30,$08,$00,$00
	.byte $80,$00,$00,$00
	.byte $30,$00,$00,$00
	.byte $00,$00,$00,$00
