.section rodata
msg:
    .word main
.section text
    .global main
    .extern getchar
main:
    push r1
    ldr r2, sp
    call getchar
    cmp r1, r5
    jne skip
    ldr r5, $66
skip:
    ldr r0, $0
    ldr sp, r0
    pop r0
    iret
