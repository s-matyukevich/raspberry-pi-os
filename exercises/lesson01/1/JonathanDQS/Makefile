#Cross compiler as I am working in a Dell XPS 15 so x86 arch
ARMGNU ?= aarch64-linux-gnu

#nostdlib not use std C library
#nostartfiles not use std startup files - we'll make them
#ffreestanding not assume std functions have std definitions
#Iinclude search for headers in include dir
#mgeneral-regs-only use general purpose regs only, might be changed later for
#context switching purposes
COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude

BUILD_DIR = build
#object files dir
SRC_DIR = src
#src coude dir

all : kernel8.img

clean :
	rm -rf $(BUILD_DIR) *.img

#To compile C and assembler files
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

#If directory for build does not exist create it
C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

#Extract from elf files as we need the binary since the elf need an OS
#Use the linker script /linker.ld to define layout of image
kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T $(SRC_DIR)/linker.ld -o $(BUILD_DIR)/kernel8.elf  $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

#Convention is to put 8 rather than modify the config.txt file arm_control works
#in the same way to boot in 64-bit mode
