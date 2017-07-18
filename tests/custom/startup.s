.segment "HEADER"
;; iNES header identifier: "NES" + $1A. Don't use a string in case
;; a non-standard charmap is setup
.byte   $4e, $45, $53, $1A
.byte   2     ; PRG code
.byte   0     ; CHR data
.byte   ((0 & $0F) << 4) | 0
.byte   (0 & $F0)

;; Filler
.byte   $00, $00, $00, $00
.byte   $00, $00, $00, $00

.segment "CODE"

startup:
	sei      ; set INTERRUPT DISABLE flag
	cld      ; clear DECIMAL flag
	ldx #$ff ; initialize...
	txs      ; the stack pointer



.segment "VECTORS"
.word startup
.word startup
.word $0000


