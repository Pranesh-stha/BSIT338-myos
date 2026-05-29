# stage2 Makefile (GCC cross-compiler version)
# All toolchain vars (TARGET_CC, TARGET_CFLAGS, etc.) come from config.mk
# via the top-level Makefile's `export`.

TARGET_CFLAGS   += -I.
TARGET_LDFLAGS  += -T linker.ld -nostdlib

SOURCES_C   := $(wildcard *.c)
SOURCES_ASM := $(wildcard *.asm)

OBJECTS_C   := $(patsubst %.c,   $(BUILD_DIR)/stage2/c/%.o,   $(SOURCES_C))
OBJECTS_ASM := $(patsubst %.asm, $(BUILD_DIR)/stage2/asm/%.o, $(SOURCES_ASM))

.PHONY: all clean always

all: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: $(OBJECTS_ASM) $(OBJECTS_C)
	$(TARGET_LD) $(TARGET_LDFLAGS) -Wl,-Map=$(BUILD_DIR)/stage2.map -o $(BUILD_DIR)/stage2.elf $^ $(TARGET_LIBS)
	$(TARGET_OBJCOPY) -O binary $(BUILD_DIR)/stage2.elf $@

$(BUILD_DIR)/stage2/c/%.o: %.c always
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/stage2/asm/%.o: %.asm always
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

always:
	mkdir -p $(BUILD_DIR)/stage2/c
	mkdir -p $(BUILD_DIR)/stage2/asm

clean:
	rm -f $(BUILD_DIR)/stage2.bin $(BUILD_DIR)/stage2.elf $(BUILD_DIR)/stage2.map
	rm -rf $(BUILD_DIR)/stage2
