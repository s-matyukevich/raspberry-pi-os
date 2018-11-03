# Option "-mgeneral-regs-only"

I couldn't get the difference between compiling with "-mgeneral-regs-only" and without it.
The disassembled code with the parameter is in "dis_with.txt", and without "dis_without.txt".
Used command to disassembly: "aarch64-linux-gnu-objdump -d build/*.o > file.txt".
Used command to compare the two files: "diff -s dis_with.txt dis_without.txt", and output "Files dis_with.txt and dis_without.txt are identical".