SECTIONS
{
    .text :
    {
        KEEP(*(.text.boot))
        *(.text)
    }
    .rodata :{ *(.rodata) }
    .data :{ *(.data) }
    .bss : { *(.bss*) } 
}

