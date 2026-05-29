# kernel Makefile (GCC cross-compiler version)
# All toolchain vars (TARGET_CC, TARGET_CFLAGS, etc.) come from config.mk
# via the top-level Makefile's `export`.

TARGET_CFLAGS   += -I.
TARGET_LDFLAGS  += -T linker.ld -nostdlib

# Recurse into subfolders (e.g. arch/i686/) instead of just the top dir.
SOURCES_C   = $(shell find . -name '*.c')
SOURCES_ASM = $(shell find . -name '*.asm')
HEADERS_C   = $(shell find . -name '*.h')

# find prefixes paths with ./ — strip it so objects land under build/kernel/...
OBJECTS_C   = $(patsubst ./%.c,   $(BUILD_DIR)/kernel/c/%.o,   $(SOURCES_C))
OBJECTS_ASM = $(patsubst ./%.asm, $(BUILD_DIR)/kernel/asm/%.o, $(SOURCES_ASM))

.PHONY: all clean

all: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(TARGET_LDFLAGS) -Wl,-Map=$(BUILD_DIR)/kernel.map -o $(BUILD_DIR)/kernel.elf $^ $(TARGET_LIBS)
	$(TARGET_OBJCOPY) -O binary $(BUILD_DIR)/kernel.elf $@

$(BUILD_DIR)/kernel/c/%.o: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.o: %.asm
	@mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.map
	rm -rf $(BUILD_DIR)/kernel
