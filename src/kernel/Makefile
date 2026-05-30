# kernel Makefile (GCC cross-compiler version)
# All toolchain vars (TARGET_CC, TARGET_CFLAGS, etc.) come from config.mk
# via the top-level Makefile's `export`.

TARGET_CFLAGS   += -I.
TARGET_LDFLAGS  += -T linker.ld -nostdlib

# ----- Generated ISR stubs ------------------------------------------------
# Auto-built by build_scripts/generate_isrs.sh. The .inc is %include'd by
# arch/i686/isr.asm; the .c provides extern decls + i686_ISR_InitializeGates.
ISR_GEN_INC := arch/i686/isrs_gen.inc
ISR_GEN_C   := ./arch/i686/isrs_gen.c
GEN_ISRS    := ../../build_scripts/generate_isrs.sh
# --------------------------------------------------------------------------

# Recurse into subfolders (e.g. arch/i686/). Exclude the generated .c so it
# doesn't appear twice once it exists (find would pick it up + we add it).
SOURCES_C   = $(shell find . -name '*.c' ! -name 'isrs_gen.c')
SOURCES_C  += $(ISR_GEN_C)
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

# Grouped target (make 4.3+): recipe runs ONCE and produces both files.
$(ISR_GEN_INC) $(ISR_GEN_C) &: $(GEN_ISRS)
	bash $(GEN_ISRS) $(ISR_GEN_INC) $(ISR_GEN_C)

# isr.asm %include's isrs_gen.inc — must be regenerated before nasm runs.
$(BUILD_DIR)/kernel/asm/arch/i686/isr.o: $(ISR_GEN_INC)

$(BUILD_DIR)/kernel/c/%.o: %.c $(HEADERS_C)
	@mkdir -p $(@D)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/kernel/asm/%.o: %.asm
	@mkdir -p $(@D)
	$(TARGET_ASM) $(TARGET_ASMFLAGS) -o $@ $<

clean:
	rm -f $(BUILD_DIR)/kernel.bin $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/kernel.map
	rm -rf $(BUILD_DIR)/kernel
	rm -f $(ISR_GEN_INC) $(ISR_GEN_C)
