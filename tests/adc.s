  .inesprg 1
  .ineschr 0
  .inesmap 0 
  .inesmir 0 

  .bank 0
  .org $8000

start:
	sei ; disable interrupts
	cld ; clear decimal flag
	ldx #$ff ; initialize ...
	txs      ; the stack pointer to $1FF

	; put some values on zeropage
	ldx #$01
	ldy #$02
	lda #$63
	sta <$00,x  ; test sta zeropage,X should store a into $0001
	lda #$65
	sta $00,y   ; test sta absolute,Y should store a into $0002

	; test the adc on those values
test:
	clv         ; clear overflow flag
	lda #$00    ; set A and also clears negative flag
	adc <$00,x  ; test adc zeropage,x should add $63 from $0001
	adc $00,y   ; test adc absolute,y should add $65 from $0002
	; the negative flag and the overflow flag should be set
	jmp test

  .bank 1
  .org $FFFA
  .dw start
  .dw start
  .dw 0
