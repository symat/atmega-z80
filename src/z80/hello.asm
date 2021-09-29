        .LI     OFF
;
;       Prints 'Hello World!' to the console
;
        .CR     Z80
        .TF     hello.z80, BIN
        .OR     $0100
P_CON   .EQ     0

        LD      C, P_CON        ; Console output port 
        LD      HL, MSG         ; Message start
        LD      B, MSG_END-MSG  ; Message length
        OTIR                    ; Print message
        HALT
        
MSG     .DB     'Hello World!', $0D, $0A
MSG_END
