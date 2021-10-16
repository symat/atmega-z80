;
;       Blinks the user LED with approx. 1Hz frequency
;
        ORG     $0100
P_LED   EQU     $01

START   LD A, 2
        OUT (P_LED), A  ; Select LED mode
        XOR A
        
BLINK   OUT (P_LED), A  ; Enable (A=1) / disable (A=0) LED

; Wait 5000000 cycles ~ 500ms @ 10MHz
        LD D, 100       
SLEEP   LD B, 171
LOOP    REPT 69
        NOP
        ENDM
        DEC B
        JR NZ, LOOP
        REPT 12
        NOP
        ENDM
        DEC D
        JR NZ, SLEEP
; The previous loop sleeps "only" 4999795 cycles
; so add 205 more :)
        LD D, 12
        LD D, 12        ; Intentional (+7 cycles)
LOOP2   DEC D           
        JR NZ, LOOP2
        NOP
        
        XOR 1           ; Toggle A
        JR BLINK
