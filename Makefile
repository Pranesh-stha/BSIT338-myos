ASM = nasm
BUILD_DIR = build
SRC_DIR = src

.PHONY: all floppy_image kernel clean always run

all: floppy_image

floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: $(BUILD_DIR)/main.bin
	cp $(BUILD_DIR)/main.bin $(BUILD_DIR)/main_floppy.img
	truncate -s 1440k $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main.bin: always
	$(ASM) $(SRC_DIR)/main.asm -f bin -o $(BUILD_DIR)/main.bin

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)/*

run: floppy_image
	qemu-system-i386 -fda $(BUILD_DIR)/main_floppy.img