BUILD_DIR = build
ASSETS_DIRS := assets
ASM_DIRS := asm asm/os asm/libultra/os
ASM_DATA_DIRS := asm/data
SRC_DIRS := $(shell find src/ -type d)

C_FILES := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
C_FILES += $(wildcard src/*.c)
S_FILES := $(foreach dir,$(ASM_DIRS),$(wildcard $(dir)/*.s))
S_DATA_FILES := $(foreach dir,$(ASM_DATA_DIRS),$(wildcard $(dir)/*.data.s))
S_RODATA_FILES := $(foreach dir,$(ASM_DATA_DIRS),$(wildcard $(dir)/*.rodata.s))
DATA_FILES := $(foreach dir,$(ASSETS_DIRS),$(wildcard $(dir)/*.bin))

# Object files
O_FILES := $(foreach file,$(C_FILES),$(BUILD_DIR)/$(file:.c=.o)) \
           $(foreach file,$(S_FILES),$(BUILD_DIR)/$(file:.s=.s.o)) \
           $(foreach file,$(S_DATA_FILES),$(BUILD_DIR)/$(file:.data.s=.data.s.o)) \
           $(foreach file,$(S_RODATA_FILES),$(BUILD_DIR)/$(file:.rodata.s=.rodata.s.o)) \
           $(foreach file,$(DATA_FILES),$(BUILD_DIR)/$(file:.bin=.bin.o)) \

##################### Compiler Options #######################
ifeq ($(shell type mips-n64-ld >/dev/null 2>/dev/null; echo $$?), 0)
  CROSS := mips-n64-
else ifeq ($(shell type mips-linux-gnu-ld >/dev/null 2>/dev/null; echo $$?), 0)
  CROSS := mips-linux-gnu-
else ifeq ($(shell type mips64-linux-gnu-ld >/dev/null 2>/dev/null; echo $$?), 0)
  CROSS := mips64-linux-gnu-
else
  CROSS := mips64-elf-
endif

AS = $(CROSS)as
LD = $(CROSS)ld
OBJDUMP = $(CROSS)objdump
OBJCOPY = $(CROSS)objcopy

CC = tools/ido_recomp/linux/5.3/cc
CC_OLD = tools/ido_recomp/linux/7.1/cc

DEFINE_CFLAGS = -D_LANGUAGE_C -D_FINALROM -DF3DEX_GBI_2 -D_MIPS_SZLONG=32
INCLUDE_CFLAGS = -I . -I include
ASFLAGS = -EB -mtune=vr4300 -march=vr4300 -Iinclude -modd-spreg
CFLAGS  = -G0 -mips2 -non_shared -Xfullwarn -Xcpluscomm -Wab,-r4300_mul $(DEFINE_FLAGS) $(INCLUDE_CFLAGS) -DF3DEX_GBI_2
LDFLAGS = -T undefined_funcs_auto.txt -T undefined_syms_auto.txt -T $(BUILD_DIR)/$(LD_SCRIPT) -Map $(BUILD_DIR)/$(TARGET).map --no-check-sections

OPTFLAGS := -O2 -g3

GCC_CFLAGS = -Wall $(DEFINE_CFLAGS) $(INCLUDE_CFLAGS) -fno-PIC -fno-zero-initialized-in-bss -fno-toplevel-reorder -Wno-missing-braces -Wno-unknown-pragmas
CC_CHECK = gcc -fsyntax-only -fno-builtin -nostdinc -fsigned-char -m32 $(GCC_CFLAGS) -std=gnu90 -Wall -Wextra -Wno-format-security -Wno-main -DNON_MATCHING -DAVOID_UB

TARGET = yoshisstory
LD_SCRIPT = $(TARGET).ld
SPLAT_YAML = $(TARGET).yaml
SPLAT_CMD = python3 ./tools/splat/split.py $(SPLAT_YAML)

######################## Targets #############################
build/src/os/O1/%.o: OPTFLAGS := -O1
build/src/%.o: CC := python3 tools/asm_processor/build.py $(CC) -- $(AS) $(ASFLAGS) --
build/asm/%.o: ASFLAGS += -mips3 -mabi=32

default: all

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).z64 verify

clean:
	rm -rf $(BUILD_DIR)

split: $(SPLAT_YAML)
	$(SPLAT_CMD)

setup: clean split
	
$(BUILD_DIR):
	@mkdir $(BUILD_DIR)
	@mkdir $(BUILD_DIR)/asm
	@mkdir $(BUILD_DIR)/asm/data
	@mkdir $(BUILD_DIR)/assets

$(BUILD_DIR)/%.o: %.c
	@$(CC_CHECK) -MMD -MP -MT $@ -MF $(BUILD_DIR)/$*.d $<
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@$(CC_CHECK) -MMD -MP -MT $@ -MF $(BUILD_DIR)/$*.d $<
	$(CC) -c $(CFLAGS) $(OPTFLAGS) -o $@ $^

$(BUILD_DIR)/%.s.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/%.bin.o: %.bin
	$(LD) -r -b binary -o $@ $<

$(LD_SCRIPT): $(SPLAT_YAML)
	$(SPLAT_CMD)
	
$(BUILD_DIR)/$(LD_SCRIPT): $(LD_SCRIPT)
	@mkdir -p $(shell dirname $@)
	cpp -P -DBUILD_DIR=$(BUILD_DIR) -o $@ $<

$(BUILD_DIR)/$(TARGET).elf: $(BUILD_DIR)/$(LD_SCRIPT) $(O_FILES)
	$(LD) $(LDFLAGS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) $< $@ -O binary

$(BUILD_DIR)/$(TARGET).z64: $(BUILD_DIR)/$(TARGET).bin
	@cp $< $@

verify: $(BUILD_DIR)/$(TARGET).z64
	md5sum -c checksum.md5

.PHONY: all clean default split setup
