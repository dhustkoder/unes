.include "startup.s"

.segment "CODE"

_entry:
	lda #$01
	sbc #$ff
@forever:
	jmp @forever

