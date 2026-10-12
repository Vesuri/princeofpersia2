# Empty scaffold. Explicit sources; flags and header dependencies invalidate objects.
SHELL := /bin/bash
.DEFAULT_GOAL := all
CC = m68k-amiga-elf-gcc
AS = m68k-amiga-elf-as
LD = m68k-amiga-elf-ld
SUPPORT ?= $(HOME)/.local/support
VASM ?= $(HOME)/.local/vasmm68k_mot
FW = ../src/platform/amiga/framework
OUT = out/PrinceOfPersia2
COMMON = -g -m68020 -mtune=68020 -msoft-float -O2 -nostdlib -ffunction-sections -fdata-sections -fno-tree-loop-distribution -MMD -MP
CXXFLAGS = $(COMMON) -std=gnu++17 -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fcheck-new -fpermissive -Wno-write-strings -DPOP2_PLATFORM_AMIGA $(EXTRA_DEFINES) -include $(FW)/SASCCompat.h -I$(FW) -I$(FW)/compat-include -I../src
FW_NAMES = AmigaHardware Bitmap CopperList Palette Palette24Bit Sprite Util
CXX_SRC = ../src/main.cpp ../src/platform/amiga/PlatformAmiga.cpp ../src/platform/amiga/GCCRuntime.cpp $(addprefix $(FW)/,$(addsuffix .cpp,$(FW_NAMES))) $(wildcard ../src/mac/*.cpp)
ASM_NAMES = AmigaHardwareAssembler BitmapAssembler CopperListAssembler UtilAssembler
OBJS = $(addprefix obj/,$(notdir $(CXX_SRC:.cpp=.o))) $(addprefix obj/,$(addsuffix .o,$(ASM_NAMES))) obj/Startup.o obj/version.o obj/gcc8_c_support.o obj/gcc8_a_support.o
PROBES = g_scaffoldStage g_scaffoldFields g_scaffoldError g_savedDMA g_actualDMA g_savedIRQ g_actualIRQ g_viewRestored g_vectorRestored
COMMA := ,
vpath %.cpp ../src ../src/mac ../src/platform/amiga $(FW)
.PHONY: all verify FORCE audit
all: verify $(OUT).exe audit
verify:
	@python3 ../tools/scaffold_inputs.py build
obj out:
	@mkdir -p $@
# FORCE checks flags; unchanged contents leave timestamp untouched.
obj/flags: FORCE | obj
	@printf '%s\n' '$(CC) $(CXXFLAGS) $(COMMON) $(VASM)' > obj/flags.new
	@cmp -s obj/flags.new $@ || cp obj/flags.new $@
$(OBJS): obj/flags | obj verify
obj/%.o: %.cpp
	$(CC) $(CXXFLAGS) -c "$<" -o "$@"
obj/Startup.o: ../src/platform/amiga/Startup.c
	$(CC) $(COMMON) -c $< -o $@
obj/gcc8_c_support.o: $(SUPPORT)/gcc8_c_support.c
	$(CC) $(COMMON) -D_start=unused_shared_start -c "$<" -o $@
obj/gcc8_a_support.o: $(SUPPORT)/gcc8_a_support.s
	$(AS) -mcpu=68020 -g --register-prefix-optional "$<" -o $@
obj/version.o: ../src/platform/amiga/version.s ../VERSION
	$(AS) -mcpu=68020 -g "$<" -o $@
$(addprefix obj/,$(addsuffix .o,$(ASM_NAMES))): obj/%.o: $(FW)/%.s
	$(VASM) -m68020 -Felf -quiet -x -no-opt -nowarn=62 "$<" -o $@
$(OUT).elf: $(OBJS) | out
	$(CC) $(COMMON) -Wl,--emit-relocs,--gc-sections,-Ttext=0,-Map=$(OUT).map $(addprefix -Wl$(COMMA)--undefined=,$(PROBES)) $(OBJS) -o $@
$(OUT).exe: $(OUT).elf
	elf2hunk $< $@ -s
# Partial-link every compiled source without GC, so dormant missing bridges fail audit.
audit: $(OUT).exe
	$(LD) -r $(OBJS) -o obj/all.o
	@python3 ../tools/check_scaffold_link.py $(OUT).elf obj/all.o $(PROBES)
	@python3 ../tools/build_receipt.py write '$(EXTRA_DEFINES)'
-include $(wildcard obj/*.d)
