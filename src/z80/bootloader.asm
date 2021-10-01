;
;       Bootloader which loads the actual code into memory then executes it
;
        ORG     $0000
PROG    EQU     $0100

B_START XOR A
        LD C, A
        IN D, (C)
        LD HL, PROG
        LD B, A
LOOP    INIR
        DEC D
        JR NZ, LOOP
        JP PROG
B_END

IF B_END - B_START > 16
        .ERROR "Bootloader size exceeds 16 bytes!"
ENDIF
