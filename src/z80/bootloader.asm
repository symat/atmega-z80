        .LI     OFF
;
;       Bootloader which loads the actual code into memory then executes it
;
        .CR     Z80
        .TF     bootloader.z80, BIN
        .OR     $0000
PROG    .EQ     $0100

START   XOR A
        LD C, A
        IN D, (C)
        LD HL, PROG
        LD B, A
.LOOP   INIR
        DEC D
        JR NZ,.LOOP
        JP PROG
        