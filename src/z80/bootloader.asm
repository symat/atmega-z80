;
;       Bootloader which loads the actual code into memory then executes it
;
        ORG     $0000
PROG    EQU     $0100

START   XOR A
        LD C, A
        IN D, (C)
        LD HL, PROG
        LD B, A
LOOP    INIR
        DEC D
        JR NZ, LOOP
        JP PROG
