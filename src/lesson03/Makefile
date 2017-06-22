ARMGNU ?= aarch64-linux-gnu

#COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -Iinclude 
COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude 

all : kernel7.img

clean :
	rm -f src/*.o *.img 

src/%.o: src/%.c
	$(ARMGNU)-gcc $(COPS) -c $< -o $@

src/%.o: src/%.S
	$(ARMGNU)-as $< -o $@

C_FILES = $(wildcard src/*.c)
ASM_FILES = $(wildcard src/*.S)
OBJ_FILES = $(C_FILES:.c=.o)
OBJ_FILES += $(ASM_FILES:.S=.o)

kernel7.img: src/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T src/linker.ld -o kernel7.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy kernel7.elf -O binary kernel7.img
	rm kernel7.elf 
