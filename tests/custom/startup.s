.segment "HEADER"
;; iNES header identifier: "NES" + $1A. Don't use a string in case
;; a non-standard charmap is setup
.byte   $4e, $45, $53, $1A
.byte   1     ; PRG code
.byte   0     ; CHR data
.byte   ((0 & $0F) << 4) | 0
.byte   (0 & $F0)

;; Filler
.byte   $00, $00, $00, $00
.byte   $00, $00, $00, $00


.segment "CODE"

_startup:
	sei      ; set INTERRUPT DISABLE flag
	cld      ; clear DECIMAL flag
	ldx #$ff ; initialize...
	txs      ; the stack pointer
	jmp _entry

_nmi:
	pha
	lda #$01
	pla
	rti
_irq:
	pha
	lda #$02
	pla
	rti


.segment "VECTORS"
.word _nmi
.word _startup
.word _irq

