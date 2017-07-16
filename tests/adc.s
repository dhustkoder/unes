  .inesprg 1
  .ineschr 0
  .inesmap 0 
  .inesmir 0 

  .bank 0
  .org $8000

start:
	ldx #$02
	ldy #$04
	lda #$10
	sta <$00,x
	lda #$20
	sta $00,y

	; adc tests now
test:
	lda #$00
	adc <$00,x
	adc $00,y
	jmp test

  .bank 1
  .org $FFFA
  .dw start
  .dw start
  .dw 0
