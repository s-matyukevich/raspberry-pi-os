ARMGNU ?= aarch64-linux-gnu

C_FLAGS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude 
ASM_FLAGS = -Iinclude

BUILD_DIR = build
SRC_DIR = src

all : kernel8.img 

clean :
	rm -rf $(BUILD_DIR) *.img

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
	mkdir -p $(@D)
	$(ARMGNU)-gcc $(C_FLAGS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
	$(ARMGNU)-gcc $(ASM_FLAGS) -MMD -c $< -o $@

C_FILES = $(wildcard $(SRC_DIR)/*.c)
ASM_FILES = $(wildcard $(SRC_DIR)/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES  = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel8.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
	$(ARMGNU)-ld -T src/linker.ld -o $(BUILD_DIR)/kernel8.elf $(OBJ_FILES)
	$(ARMGNU)-objcopy $(BUILD_DIR)/kernel8.elf -O binary kernel8.img

