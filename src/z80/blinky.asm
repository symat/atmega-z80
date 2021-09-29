;
;       Blinks the user LED with approx. 1Hz frequency
;
        ORG     $0100
P_LED   EQU     $01

START   LD A, 2
        OUT (P_LED), A  ; Select LED mode
        XOR A
        
BLINK   OUT (P_LED), A  ; Enable (A=1) / disable (A=0) LED

        LD D, 100       ; Wait 4999700 cycles ~ 500ms @ 10MHz
SLEEP   LD B, 170
LOOP    REPT 70
        NOP
        ENDM
        DEC B
        JP NZ, LOOP
        DEC D
        JP NZ, SLEEP
        
        XOR 1           ; Toggle A
        JP BLINK
