.include "startup.s"

.segment "CODE"

NUM_TESTS = 1

_entry:
	ldx #$00
	ldy #$00
@loop:
	lda @add_values,x
	inx
	adc @add_values,x
	php ; save current flags into stack
	inx
	cmp @a_check_values,y
	bne @failure
	pla ; pull the saved flags to do cmp
	cmp @p_check_values,y
	bne @failure
	iny
	tya
	cmp #NUM_TESTS
	bne @loop
@success:
	lda #0
	jmp @success
@failure:
	lda #-1
	jmp @failure


.segment "RODATA"

@add_values:
	.byte $63
	.byte $65
@p_check_values:
	.byte $f4
@a_check_values:
	.byte $c8
