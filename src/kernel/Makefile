# kernel Makefile (GCC cross-compiler version)
# All toolchain vars (TARGET_CC, TARGET_CFLAGS, etc.) come from config.mk
# via the top-level Makefile's `export`.

TARGET_CFLAGS   += -I.
TARGET_LDFLAGS  += -T linker.ld -nostdlib

SOURCES_C   := $(wildcard *.c)
SOURCES_ASM := $(wildcard *.asm)

OBJECTS_C   := $(patsubst %.c,   $(BUILD_DIR)/kernel/c/%.o,   $(SOURCES_C))
OBJECTS_ASM := $(patsubst %.asm, $(BUILD_DIR)/kernel/asm/%.o, $(SOURCES_ASM))

.PHONY: all clean always

all: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(TARGET_LDFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $(BUILD_DIR)/kernel.elf $^ $(TARGET_LIBS)
	$(TARGET_OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

$(BUILD_DIR)/kernel/c/%.o: %.c always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.o: %.asm always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/kernel/c
	mkdir -p $(BUILD_DIR)/kernel/asm

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.map
	rm -rf $(BUILD_DIR)/kernel
