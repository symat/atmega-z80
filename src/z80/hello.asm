;
;       Prints 'Hello World!' to the console
;
        ORG     $0100
P_CON   EQU     0

        LD      C, P_CON          ; Console output port 
        LD      HL, MSG           ; Message start
        LD      B, MSG_END - MSG  ; Message length
        OTIR                      ; Print message
        HALT
        
MSG     DB      'Hello World!', $0D, $0A
        DB      $04               ; End of transmission
MSG_END
