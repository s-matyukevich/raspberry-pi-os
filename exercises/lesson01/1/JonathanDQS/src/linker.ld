SECTIONS
{
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) }
	bss_end = .;
}

/*OS startup code in .text.boot section so must be first
Then kernel instructions in .text, read only data in .rodata and data in .data
.bss section which should be initialized to 0, bss_begin and bss_end because
we store the size rather than the whole section and where it begins and ends
ALIGN(0x8) since ARM is byte addressable and it is a 64 bit arch*/
