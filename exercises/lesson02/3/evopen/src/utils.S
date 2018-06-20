    .globl put32
put32:
    str w1, [x0]
    ret

    .globl get32
get32:
    ldr w0, [x0]
    ret

    .globl delay
delay:
    subs x0, x0, #1
    bne delay
    ret

    .globl get_el
get_el:
    mrs     x0, CurrentEL
    lsr     x0, x0, #2
    ret



