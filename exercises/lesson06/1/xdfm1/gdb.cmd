target remote localhost:1234
add-symbol-file build/boot_s.o 0x0
x/10i $pc

