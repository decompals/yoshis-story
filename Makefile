# Build options can be changed by modifying the makefile or by building with 'make SETTING=value'.
# It is also possible to override the settings in Defaults in a file called .make_options as 'SETTING=value'.

-include .make_options

MAKEFLAGS += --no-builtin-rules

SHELL = /bin/bash
.SHELLFLAGS = -o pipefail -c

# OS Detection
ifeq ($(OS),Windows_NT)
  DETECTED_OS = windows
  MAKE = make
  VENV_BIN_DIR = Scripts
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    DETECTED_OS = linux
    MAKE = make
    VENV_BIN_DIR = bin
  endif
  ifeq ($(UNAME_S),Darwin)
    DETECTED_OS = macos
    MAKE = gmake
    VENV_BIN_DIR = bin
  endif
endif

#### Defaults ####
# Target game version. Currently only the following version is supported:
#   us   N64 USA (default)
VERSION ?= us
# If COMPARE is 1, check the output md5sum after building
COMPARE ?= 1
# If NON_MATCHING is 1, define the NON_MATCHING C flag when building
NON_MATCHING ?= 0
# if WERROR is 1, pass -Werror to CC_CHECK, so warnings would be treated as errors
WERROR ?= 0
# Keep .mdebug section in build
KEEP_MDEBUG ?= 0
# Check code syntax with host compiler
RUN_CC_CHECK ?= 1
CC_CHECK_COMP ?= gcc
# Dump build object files
OBJDUMP_BUILD ?= 0
# Disassembles matched functions and migrated data as well
FULL_DISASM ?= 0
# Number of threads to compress with
N_THREADS ?= $(shell nproc)
# MIPS toolchain prefix
CROSS ?= mips-linux-gnu-
# Python virtual environment
VENV ?= .venv
# Python interpreter
PYTHON ?= $(VENV)/$(VENV_BIN_DIR)/python3
# Emulator w/ flags
N64_EMULATOR ?=


BASEROM_DIR := baseroms/$(VERSION)
BASEROM     := $(BASEROM_DIR)/baserom.z64
TARGET      := yoshisstory

# ULTRALIB_VERSION should be H
ULTRALIB_VERSION     := I
ULTRALIB_TARGET      := libultra_rom

### Output ###

BUILD_DIR := build
ROM       := $(BUILD_DIR)/$(TARGET)-$(VERSION).z64
ELF       := $(ROM:.z64=.elf)
MAP       := $(ROM:.z64=.map)
LDSCRIPT  := $(ROM:.z64=.ld)


#### Setup ####

BUILD_DEFINES ?=

ifeq ($(VERSION),us)
    BUILD_DEFINES   += -DVERSION_US=1
else
$(error Invalid VERSION variable detected. Please use 'us')
endif


ifeq ($(NON_MATCHING),1)
    BUILD_DEFINES   += -DNON_MATCHING -DAVOID_UB
    COMPARE  := 0
endif

CPPFLAGS += -fno-dollars-in-identifiers -P
LDFLAGS  := --no-check-sections --accept-unknown-input-arch --emit-relocs

ifeq ($(DETECTED_OS), macos)
  CPPFLAGS += -xc++
endif

#### Tools ####
ifneq ($(shell type $(CROSS)ld >/dev/null 2>/dev/null; echo $$?), 0)
$(error Unable to find $(CROSS)ld. Please install or build MIPS binutils, commonly mips-linux-gnu. (or set CROSS if your MIPS binutils install uses another prefix))
endif


CC              := tools/ido/$(DETECTED_OS)/7.1/cc
CC_OLD          := tools/ido/$(DETECTED_OS)/5.3/cc


AS              := $(CROSS)as
LD              := $(CROSS)ld
OBJCOPY         := $(CROSS)objcopy
OBJDUMP         := $(CROSS)objdump
CPP             := cpp
ICONV           := iconv
ASM_PROC        := $(PYTHON) tools/asm-processor/build.py
CAT             := cat

ASM_PROC_FLAGS  := --input-enc=utf-8 --output-enc=euc-jp --convert-statics=global-with-filename

SPLAT           ?= $(PYTHON) -m splat split
SPLAT_YAML      ?= $(TARGET)-$(VERSION).yaml



IINC := -Iinclude -I.
IINC += -Ilib/ultralib/include -Ilib/ultralib/include/PR -Ilib/ultralib/include/ido

ifeq ($(KEEP_MDEBUG),0)
  RM_MDEBUG = $(OBJCOPY) --remove-section .mdebug $@
else
  RM_MDEBUG = @:
endif

# Check code syntax with host compiler
CHECK_WARNINGS := -Wall -Wextra -Wimplicit-fallthrough -Wno-unknown-pragmas -Wno-missing-braces -Wno-sign-compare -Wno-uninitialized
# Have CC_CHECK pretend to be a MIPS compiler
MIPS_BUILTIN_DEFS := -DMIPSEB -D_MIPS_FPSET=16 -D_MIPS_ISA=2 -D_ABIO32=1 -D_MIPS_SIM=_ABIO32 -D_MIPS_SZINT=32 -D_MIPS_SZPTR=32
ifneq ($(RUN_CC_CHECK),0)
#   The -MMD flags additionaly creates a .d file with the same name as the .o file.
    CC_CHECK          := $(CC_CHECK_COMP)
    CC_CHECK_FLAGS    := -MMD -MP -fno-builtin -fsyntax-only -funsigned-char -fdiagnostics-color -std=gnu89 -m32 -DNON_MATCHING -DAVOID_UB -DCC_CHECK=1
    ifneq ($(WERROR), 0)
        CHECK_WARNINGS += -Werror
    endif
else
    CC_CHECK          := @:
endif


CFLAGS           += -G 0 -non_shared -Xcpluscomm -nostdinc -Wab,-r4300_mul

WARNINGS         := -fullwarn -verbose -woff 624,649,838,712,516,513,596,564,594
ASFLAGS          := -march=vr4300 -32 -G0
COMMON_DEFINES   := -D_MIPS_SZLONG=32
GBI_DEFINES      := -DF3DEX_GBI
RELEASE_DEFINES  := -DNDEBUG -D_FINALROM
AS_DEFINES       := -DMIPSEB -D_LANGUAGE_ASSEMBLY -D_ULTRA64
C_DEFINES        := -DLANGUAGE_C -D_LANGUAGE_C
LIBULTRA_DEFINES := -DBUILD_VERSION=VERSION_$(ULTRALIB_VERSION)
ENDIAN           := -EB

OPTFLAGS         := -O2 -g3
MIPS_VERSION     := -mips2
ICONV_FLAGS      := --from-code=UTF-8 --to-code=EUC-JP

# Use relocations and abi fpr names in the dump
OBJDUMP_FLAGS := --disassemble --reloc --disassemble-zeroes -Mreg-names=32 -Mno-aliases

ifneq ($(OBJDUMP_BUILD), 0)
    OBJDUMP_CMD = $(OBJDUMP) $(OBJDUMP_FLAGS) $@ > $(@:.o=.dump.s)
    OBJCOPY_BIN = $(OBJCOPY) -O binary $@ $@.bin
else
    OBJDUMP_CMD = @:
    OBJCOPY_BIN = @:
endif

SPLAT_FLAGS ?=
ifneq ($(FULL_DISASM), 0)
	SPLAT_FLAGS += --disassemble-all
endif

#### Files ####

$(shell mkdir -p asm assets linker_scripts/$(VERSION)/auto)

SRC_DIRS      := $(shell find src -type d)
ASM_DIRS      := $(shell find asm/$(VERSION) -type d -not -path "asm/$(VERSION)/nonmatchings/*")
BIN_DIRS      := $(shell find assets -type d)

C_FILES       := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES       := $(foreach dir,$(ASM_DIRS) $(SRC_DIRS),$(wildcard $(dir)/*.s))
BIN_FILES     := $(foreach dir,$(BIN_DIRS),$(wildcard $(dir)/*.bin))
O_FILES       := $(foreach f,$(C_FILES:.c=.o),$(BUILD_DIR)/$f) \
                 $(foreach f,$(S_FILES:.s=.o),$(BUILD_DIR)/$f) \
                 $(foreach f,$(BIN_FILES:.bin=.o),$(BUILD_DIR)/$f)

ULTRALIB_DIR  := lib/ultralib
ULTRALIB_LIB  := $(ULTRALIB_DIR)/build/$(ULTRALIB_VERSION)/$(ULTRALIB_TARGET)/$(ULTRALIB_TARGET).a
LIBULTRA      := $(BUILD_DIR)/$(ULTRALIB_TARGET).a

# Automatic dependency files
DEP_FILES := $(O_FILES:.o=.d) \
             $(O_FILES:.o=.asmproc.d)

# create build directories
$(shell mkdir -p $(BUILD_DIR)/linker_scripts/$(VERSION) $(BUILD_DIR)/linker_scripts/$(VERSION)/auto $(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(BIN_DIRS),$(BUILD_DIR)/$(dir)))


# directory flags
build/src/main/O2/%.o: OPTFLAGS := -O2

# per-file flags
build/src/main/fault.o: CFLAGS += -trapuv

# cc & asm-processor
build/src/%.o: CC := $(ASM_PROC) $(ASM_PROC_FLAGS) $(CC) -- $(AS) $(ASFLAGS) --


#### Main Targets ###

all: rom

rom: $(ROM)
ifneq ($(COMPARE),0)
	@md5sum $(ROM)
	@md5sum -c $(BASEROM_DIR)/checksum.md5
endif

clean:
	$(RM) -r $(BUILD_DIR)/asm $(BUILD_DIR)/assets $(BUILD_DIR)/src $(ROM) $(ELF)

libclean:
	$(MAKE) -C lib/ultralib clean VERSION=$(ULTRALIB_VERSION) TARGET=$(ULTRALIB_TARGET)

distclean: clean libclean
	$(RM) -r $(BUILD_DIR) asm/ assets/ .splat/
	$(RM) -r linker_scripts/$(VERSION)/auto $(LDSCRIPT)
	$(MAKE) -C lib/ultralib distclean
	$(MAKE) -C tools distclean

venv:
	test -d $(VENV) || python3 -m venv $(VENV)
	$(PYTHON) -m pip install -U pip
	$(PYTHON) -m pip install -U -r requirements.txt

setup:
	$(MAKE) -C tools

lib:
	$(MAKE) -C lib/ultralib VERSION=$(ULTRALIB_VERSION) TARGET=$(ULTRALIB_TARGET) COMPARE=0 CROSS=$(CROSS) CC=../../$(CC)

extract:
	$(RM) -r asm/$(VERSION) assets/$(VERSION)
	$(CAT) yamls/$(VERSION)/header.yaml yamls/$(VERSION)/makerom.yaml yamls/$(VERSION)/main.yaml > $(SPLAT_YAML)
	$(SPLAT) $(SPLAT_FLAGS) $(SPLAT_YAML)

diff-init: rom
	$(RM) -rf expected/
	mkdir -p expected/
	cp -r $(BUILD_DIR) expected/$(BUILD_DIR)

init: distclean
	$(MAKE) venv
	$(MAKE) setup
	$(MAKE) extract
	$(MAKE) lib
	$(MAKE) all
	$(MAKE) diff-init

run: $(ROM)
ifeq ($(N64_EMULATOR),)
	$(error Emulator path not set. Set N64_EMULATOR in the Makefile, .make_options, or define it as an environment variable)
endif
	$(N64_EMULATOR) $<

.PHONY: all rom clean libclean distclean setup extract lib diff-init init venv run
.DEFAULT_GOAL := rom
# Prevent removing intermediate files
.SECONDARY:


#### Various Recipes ####

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
# TODO: update rom header checksum

# TODO: avoid using auto/undefined
$(ELF): $(LIBULTRA) $(O_FILES) $(LDSCRIPT) $(BUILD_DIR)/linker_scripts/$(VERSION)/hardware_regs.ld $(BUILD_DIR)/linker_scripts/$(VERSION)/undefined_syms.ld $(BUILD_DIR)/linker_scripts/$(VERSION)/pif_syms.ld $(BUILD_DIR)/linker_scripts/$(VERSION)/auto/undefined_syms_auto.ld $(BUILD_DIR)/linker_scripts/$(VERSION)/auto/undefined_funcs_auto.ld
	$(LD) $(LDFLAGS) -T $(LDSCRIPT) \
		-T $(BUILD_DIR)/linker_scripts/$(VERSION)/hardware_regs.ld -T $(BUILD_DIR)/linker_scripts/$(VERSION)/undefined_syms.ld -T $(BUILD_DIR)/linker_scripts/$(VERSION)/pif_syms.ld \
		-T $(BUILD_DIR)/linker_scripts/$(VERSION)/auto/undefined_syms_auto.ld -T $(BUILD_DIR)/linker_scripts/$(VERSION)/auto/undefined_funcs_auto.ld \
		-Map $(MAP) -o $@

$(LIBULTRA): $(ULTRALIB_LIB)
	cp $< $@

$(ULTRALIB_LIB):
	$(MAKE) lib

$(BUILD_DIR)/%.ld: %.ld
	$(CPP) $(CPPFLAGS) $(BUILD_DEFINES) $(IINC) $< > $@

$(BUILD_DIR)/%.o: %.bin
	$(OBJCOPY) -I binary -O elf32-big $< $@

$(BUILD_DIR)/%.o: %.s
	$(ICONV) $(ICONV_FLAGS) $< | $(AS) $(ASFLAGS) $(ENDIAN) $(IINC) -I $(dir $*) -o $@
	$(OBJDUMP_CMD)

$(BUILD_DIR)/%.o: %.c
	$(CC_CHECK) $(CC_CHECK_FLAGS) $(IINC) -I $(dir $*) $(CHECK_WARNINGS) $(BUILD_DEFINES) $(COMMON_DEFINES) $(RELEASE_DEFINES) $(GBI_DEFINES) $(LIBULTRA_DEFINES) $(C_DEFINES) $(MIPS_BUILTIN_DEFS) -o $@ $<
	$(CC) -c $(CFLAGS) $(BUILD_DEFINES) $(IINC) $(WARNINGS) $(MIPS_VERSION) $(ENDIAN) $(COMMON_DEFINES) $(RELEASE_DEFINES) $(GBI_DEFINES) $(LIBULTRA_DEFINES) $(C_DEFINES) $(OPTFLAGS) -o $@ $<
	$(OBJDUMP_CMD)
	$(RM_MDEBUG)

-include $(DEP_FILES)

# Print target for debugging
print-% : ; $(info $* is a $(flavor $*) variable set to [$($*)]) @true
