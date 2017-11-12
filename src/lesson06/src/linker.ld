SECTIONS
{
	. = 0xffff000000000000;
	.text.boot : { *(.text.boot) }
 	. = ALIGN(0x00001000);
	user_begin = .;
	.text.user : { src/user* (.text) }
	.rodata.user : { src/user* (.rodata) }
	.data.user : {	src/user* (.data) }
	.bss.user : {	src/user* (.bss) }
	user_end = .;
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
 	. = ALIGN(0x8);
	bss_begin = .;
    .bss : { *(.bss*) } 
	bss_end = .;
 	. = ALIGN(0x00001000);
	pg_dir = .;
    .data.pgd : { . += (4 * (1 << 12)); }
}

