include build_scripts/config.mk

SRC_DIR := src

.PHONY: all floppy_image kernel bootloader stage1 stage2 clean always run

all: floppy_image

#
# Floppy image
#
floppy_image: $(BUILD_DIR)/main_floppy.img

$(BUILD_DIR)/main_floppy.img: bootloader kernel
	dd if=/dev/zero of=$(BUILD_DIR)/main_floppy.img bs=512 count=2880
	mkfs.fat -F 12 -n "DHOKAOS" $(BUILD_DIR)/main_floppy.img
	dd if=$(BUILD_DIR)/stage1.bin of=$(BUILD_DIR)/main_floppy.img conv=notrunc
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/stage2.bin "::stage2.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img $(BUILD_DIR)/kernel.bin "::kernel.bin"
	mcopy -i $(BUILD_DIR)/main_floppy.img test.txt "::test.txt"

#
# Bootloader
#
bootloader: stage1 stage2

stage1: $(BUILD_DIR)/stage1.bin

$(BUILD_DIR)/stage1.bin: always
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1

stage2: $(BUILD_DIR)/stage2.bin

$(BUILD_DIR)/stage2.bin: always
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2

#
# Kernel
#
kernel: $(BUILD_DIR)/kernel.bin

$(BUILD_DIR)/kernel.bin: always
	$(MAKE) -C $(SRC_DIR)/kernel

#
# Always
#
always:
	mkdir -p $(BUILD_DIR)

#
# Clean
#
clean:
	$(MAKE) -C $(SRC_DIR)/bootloader/stage1 clean
	$(MAKE) -C $(SRC_DIR)/bootloader/stage2 clean || true
	$(MAKE) -C $(SRC_DIR)/kernel clean
	rm -rf $(BUILD_DIR)/*

#
# Run
#
run: floppy_image
	qemu-system-i386 -fda $(BUILD_DIR)/main_floppy.img

# Same, but route PC-speaker output through PulseAudio so the
# `beep` / `melody` commands are audible. Requires QEMU built with PA
# support and a working host audio setup (WSLg provides one).
run-sound: floppy_image
	qemu-system-i386 -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 \
		-fda $(BUILD_DIR)/main_floppy.img
