.setcpu "65C02"
.debuginfo
.segment "BIOS"

KBD             = $D010         ;  PIA.A keyboard input
KBDCR           = $D011         ;  PIA.A keyboard control register
DSP             = $D012         ;  PIA.B display output register
DSPCR           = $D013         ;  PIA.B display control register

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
                cmp #$1B        ; If an <ESC>, don't echo
                beq @was_esc
                jsr CHROUT      ; Display character.
@was_esc:
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
                
                
.segment "RESETVEC"
                .word   $0F00           ; NMI vector
                .word   RESET           ; RESET vector
                .word   $0000           ; IRQ vector


