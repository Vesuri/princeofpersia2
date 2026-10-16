    .text
    .globl _start
_start:
    movem.l %d2-%d7/%a2-%a6,-(%sp)
    jsr amiga_main
    movem.l (%sp)+,%d2-%d7/%a2-%a6
    rts
