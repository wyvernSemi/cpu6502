.setcpu "65C02"
.debuginfo
.segment "BIOS"

; Input a character from the serial interface.
; On return, carry flag indicates whether a key was pressed
; If a key was pressed, the key value will be in the A register
;
; Modifies: flags, A
MONRDKEY:
CHRIN:
                lda KBDCR       ; Key ready?
                and #$80
                beq @no_keypressed
                lda KBD         ; Load character. B7 should be ‘1’.
                and #$7F
                jsr CHROUT      ; Display character.
                sec
                rts
@no_keypressed:
                clc
                rts


; Output a character (from the A register) to the serial interface.
;
; Modifies: flags
MONCOUT:
CHROUT:
                pha
WAITOP:         bit DSP         ; DA bit (B7) cleared yet?
                bmi WAITOP      ; No, wait for display.
                ora #$80
                sta DSP         ; Output character. Sets DA.
                pla
                rts
                
                
.segment "WOZMON"
.org $FF00
.include "../wozmon/wozmon.asm"


