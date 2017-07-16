 .include "setup.s"

 .bank entry_bank
 .org entry_addr

entry:
	 lda #$00
 .loop:  jsr function
	 jmp .loop
		



function:
	adc #$10
	rts

