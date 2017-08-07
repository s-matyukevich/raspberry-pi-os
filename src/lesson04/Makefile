ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only

all : kernel7.img

clean :
	rm -f src/*.o *.img 

src/%_c.o: src/%.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

src/%_s.o: src/%.S
	$(ARMGNU)-as $< -o $@

C_FILES = $(wildcard src/*.c)
ASM_FILES = $(wildcard src/*.S)
OBJ_FILES = $(C_FILES:.c=_c.o)
OBJ_FILES += $(ASM_FILES:.S=_s.o)

kernel7.img: src/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T src/linker.ld -o kernel7.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy kernel7.elf -O binary kernel7.img
	#rm kernel7.elf 
