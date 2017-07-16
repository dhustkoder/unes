 .inesprg 1
 .ineschr 0
 .inesmap 0 
 .inesmir 0

entry_bank .equ 0

 .bank 0
 .org $8000

setup:	sei      ; disable interrupts
	cld      ; clear decimal flag
	ldx #$ff ; initialize ...
	txs      ; the stack pointer to $1FF

entry_addr:      ; main code starts here


 .bank 1
 .org $FFFA
 .dw setup
 .dw setup
 .dw $00

