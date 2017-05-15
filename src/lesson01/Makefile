ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 

all : build/kernel7.img

clean :
	rm -f build/*

build/boot.o : src/boot.S
	$(ARMGNU)-as src/boot.S -o build/boot.o

build/kernel.o : src/kernel.c
	$(ARMGNU)-gcc $(COPS) -c src/kernel.c -o build/kernel.o

build/kernel7.img : src/linker.ld build/boot.o build/kernel.o 
	$(ARMGNU)-ld -T src/linker.ld -o build/kernel7.elf  build/kernel.o build/boot.o
	$(ARMGNU)-objcopy build/kernel7.elf -O binary build/kernel7.img
