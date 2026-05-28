BUILD_DIR?=build/
ASM?=nasm

SOURCES=$(wildcard *.asm)
OBJECTS=$(patsubst %.asm, $(BUILD_DIR)/bootloader/stage2/%.obj, $(SOURCES))

.PHONY: all stage2 clean always

all: stage2

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	$(ASM) main.asm -f bin -o $(BUILD_DIR)/stage2.bin

always:
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(BUILD_DIR)/stage2.bin
