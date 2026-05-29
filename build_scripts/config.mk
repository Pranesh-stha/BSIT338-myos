#
# Build configuration
# Single source of truth for paths, toolchain, and flags.
#

# Build output directory (absolute path)
export BUILD_DIR := $(abspath build)

# Cross-compiler installation (already built at /root/opt/cross)
export TOOLCHAIN_PREFIX := /root/opt/cross
export TARGET := i686-elf

# Add cross-compiler to PATH for sub-makes
export PATH := $(TOOLCHAIN_PREFIX)/bin:$(PATH)

#
# Host toolchain (rarely used — for tools that run on the dev machine)
#
export ASM    := nasm
export CC     := gcc
export CXX    := g++
export LD     := gcc

export CFLAGS    := -std=c99 -g
export CXXFLAGS  := -g
export ASMFLAGS  :=
export LDFLAGS   :=
export LIBS      :=

#
# Target toolchain (the cross-compiler — used for stage2 and kernel)
#
export TARGET_ASM    := nasm
export TARGET_CC     := $(TARGET)-gcc
export TARGET_CXX    := $(TARGET)-g++
export TARGET_LD     := $(TARGET)-gcc
export TARGET_OBJCOPY:= $(TARGET)-objcopy

# CFLAGS for target code:
#   -std=c99       : use C99 (matches what we used before)
#   -g             : debug symbols (helps when debugging in QEMU/bochs)
#   -ffreestanding : tells GCC we are not on a hosted OS (no libc assumed)
#   -nostdlib      : do not link any standard library
#   -Wall          : sensible warnings
export TARGET_CFLAGS    := -std=c99 -g -ffreestanding -nostdlib -Wall
export TARGET_CXXFLAGS  := -g -ffreestanding -nostdlib
export TARGET_ASMFLAGS  := -f elf
export TARGET_LDFLAGS   :=

# libgcc gives us free 64-bit division, varargs helpers, etc.
# This is one of the main reasons we switched away from OpenWatcom.
export TARGET_LIBS      := -lgcc
