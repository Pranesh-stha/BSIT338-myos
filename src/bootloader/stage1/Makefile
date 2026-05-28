BUILD_DIR?=build/
ASM?=nasm

SOURCES=$(wildcard *.asm)
OBJECTS=$(patsubst %.asm, $(BUILD_DIR)/bootloader/stage1/%.obj, $(SOURCES))

.PHONY: all stage1 clean always

all: stage1

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	$(ASM) main.asm -f bin -o $(BUILD_DIR)/stage1.bin

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(BUILD_DIR)/stage1.bin
