.include "startup.s"

.segment "CODE"

_entry:

@test_mirrors_1:
	; init $0002 - $0800, $00 - $01 used for indirect addressing
	ldx #$00
	ldy #$04
	lda #$00
	sta $01
	sta $00
	sta $03
	sta $02
@loop_init_ram:
	sta ($00),y
	clc
	adc #$01
	iny
	bne @loop_init_ram
	inc $01
	tax
	lda $01
	cmp #$08
	beq @cmp_ram_mirror
	txa
	jmp @loop_init_ram

@cmp_ram_mirror:
	lda #$00
	sta $00
	sta $01
	sta $02
	lda #$08
	sta $03
@cmp_ram_mirror_reset_y:
	ldy #$04
@loop_cmp_ram_mirror:
	lda ($02),y
	cmp ($00),y
	bne @failure
	iny
	bne @loop_cmp_ram_mirror
	inc $01
	lda $01
	cmp #$08
	bne @skip_01_reset
	lda #$00
	sta $01
@skip_01_reset:
	inc $03
	lda $03
	cmp #$20
	beq @success
	jmp @cmp_ram_mirror_reset_y

@failure:
	brk
	lda #-1
	jmp @failure

@success:
	brk
	lda #0
	jmp @success


