        .LI     OFF
;
;       Blinks the user LED with approx. 1Hz frequency
;
        .CR     Z80
        .TF     blinky.z80, BIN
        .OR     $0100
P_LED   .EQ     $01

NOP10   .MA
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        .EM

START   LD A, 2
        OUT (P_LED), A  ; Select LED mode
        XOR A
        
BLINK   OUT (P_LED),A   ; Enable (A=1) / disable (A=0) LED

        LD D,100        ; Wait 4999700 cycles ~ 500ms @ 10MHz
SLEEP   LD B,170
.LOOP   >NOP10
        >NOP10
        >NOP10
        >NOP10
        >NOP10
        >NOP10
        >NOP10
        DEC B
        JP NZ,.LOOP
        DEC D
        JP NZ,SLEEP
        
        XOR 1           ; Toggle A
        JP BLINK
