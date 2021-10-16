.global a,c
.extern b
.section text
    jmp a
    jmp e
    jmp b
    jmp d
d: .word d
    ldr r1, b
    ldr r2, $c
    ldr r6, %e
.skip 10
.equ e, 0x0A
.word c
a: .word b
c: .skip 8
.end