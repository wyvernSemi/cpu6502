.segment "CODE"
; ----------------------------------------------------------------------------
; SEE IF CONTROL-C TYPED
; ----------------------------------------------------------------------------

ISCNTC:
        jsr MONRDKEY
        bcc not_cntc
        cmp #$1B         ; remapped to ESC
        bne not_cntc
        jmp is_cntc

not_cntc:
        rts

is_cntc:
        ; Fall through

;!!! runs into "STOP"
