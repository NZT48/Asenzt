.extern hello
.extern world
.section first_section
msg:
    .word main, world
.section text
    .global main
    jmp dest
    .extern getchar
main:
    push r1
    ldr r2, sp
    call getchar
    cmp r1, r5
    jne dest
    ldr r5, $66
dest:
    ldr r0, $0
    jgt *[r0 + main]
    pop r0
    iret
