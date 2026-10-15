# Release by default. VERIFY=1 enables expensive runtime diagnostics.
VERIFY ?= 0
PROFILE ?= 0
ifeq ($(filter 0 1,$(PROFILE)),)
$(error PROFILE must be 0 or 1)
endif
ifeq ($(PROFILE),1)
ifneq ($(VERIFY),0)
$(error PROFILE=1 requires VERIFY=0 for measurements without expensive verification)
endif
endif
ifeq ($(filter 0 1,$(VERIFY)),)
$(error VERIFY must be 0 or 1)
endif
EXTRA_DEFINES ?= -DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO
PROFILE_ALLOWED = $(if $(filter 1,$(PROFILE)),-DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_PROBE -DPOP2_INTEGRATION_NO_RIGHT -DPOP2_OPENING_PROFILE)
ifeq ($(VERIFY),0)
ifneq ($(filter-out $(PROFILE_ALLOWED) -DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO,$(filter -DPOP2_%,$(EXTRA_DEFINES))),)
$(error Diagnostic POP2 defines require VERIFY=1; PROFILE=1 permits only integration, timing and opening-profile measurement probes)
endif
endif
VERIFY_DEFINES = $(if $(filter 1,$(VERIFY)),-DPOP2_VERIFY)
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
CXXFLAGS = $(COMMON) -std=gnu++17 -fno-rtti -fno-exceptions -fno-use-cxa-atexit -fcheck-new -fpermissive -Wno-write-strings -DPOP2_PLATFORM_AMIGA -DAMIGAXDEV_RESOURCE_DIRECTORY_INDEXED -DAMIGAXDEV_RESOURCE_DIRECTORY_MAP_LIMIT $(EXTRA_DEFINES) $(VERIFY_DEFINES) -include $(FW)/SASCCompat.h -I$(FW) -I$(FW)/compat-include -I../src
FW_NAMES = AmigaHardware Bitmap CopperList Palette Palette24Bit Sprite Util
CXX_SRC = ../src/main.cpp ../src/platform/amiga/PlatformAmiga.cpp ../src/platform/amiga/PrinceScreen.cpp ../src/platform/amiga/GCCRuntime.cpp ../src/platform/amiga/CoreRuntime.cpp ../src/platform/amiga/NativeAudio.cpp ../src/platform/amiga/PaulaAudio.cpp ../src/platform/amiga/NativeUI.cpp ../src/platform/amiga/NativeInput.cpp ../src/platform/amiga/NativeFiles.cpp ../src/platform/amiga/NativeDisplay.cpp ../src/platform/amiga/NativeResources.cpp ../src/platform/amiga/ResourceStage.cpp $(addprefix $(FW)/,$(addsuffix .cpp,$(FW_NAMES))) $(wildcard ../src/mac/*.cpp)
ASM_NAMES = AmigaHardwareAssembler BitmapAssembler CopperListAssembler UtilAssembler
OBJS = $(addprefix obj/,$(notdir $(CXX_SRC:.cpp=.o))) $(addprefix obj/,$(addsuffix .o,$(ASM_NAMES))) obj/Startup.o obj/version.o obj/DisplayC2P.o obj/gcc8_c_support.o obj/gcc8_a_support.o
PROBES = g_inputVectorRestored g_scaffoldStage g_scaffoldFields g_scaffoldError g_savedDMA g_actualDMA g_savedIRQ g_actualIRQ g_viewRestored g_vectorRestored
ifneq (,$(findstring -DPOP2_CORE,$(EXTRA_DEFINES)))
OBJS += obj/MacEntry.o
.PHONY: verify-core
verify: verify-core
verify-core:
	@python3 ../tools/core_inputs.py --verify
PROBES += g_coreCodeAudit g_coreTrace g_coreCount g_coreA5 g_coreBelow g_coreSegments g_coreInitial g_coreStopTrap g_coreStopSegment g_coreStopOffset g_coreZoneBytes g_coreHeapOK g_coreVectorRestored g_coreReason
ifneq (,$(filter -DPOP2_INTEGRATION_PROBE,$(EXTRA_DEFINES)))
PROBES += g_integrationRightReads g_integrationEscReads g_integrationSpaceEvents
endif
obj/CoreRuntime.o: ../build/core/Inputs.h
ifneq (,$(filter -DPOP2_GAMEPLAY_ROUTE_PROBE,$(EXTRA_DEFINES)))
GAMEPLAY_LEVEL ?= 1
override EXTRA_DEFINES += -DPOP2_GAMEPLAY_ROUTE_LEVEL=$(GAMEPLAY_LEVEL)
obj/CoreRuntime.o: ../build/gameplay/RouteInputs.h
verify: verify-gameplay-route
.PHONY: verify-gameplay-route
verify-gameplay-route:
	@python3 ../tools/gameplay_route_inputs.py --level $(GAMEPLAY_LEVEL) $(if $(GAMEPLAY_REFERENCE),--reference "$(GAMEPLAY_REFERENCE)") $(if $(GAMEPLAY_MANIFEST),--manifest "$(GAMEPLAY_MANIFEST)") --verify
endif
ifneq (,$(filter -DPOP2_AUDIO,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/audio/AudioInputs.h
ifneq (,$(filter -DPOP2_AUDIO_PROBE,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/audio/AudioReplay.h
verify: verify-audio-replay
.PHONY: verify-audio-replay
verify-audio-replay:
	@python3 ../tools/audio_replay.py --verify
PROBES += g_audioCalls g_audioDone g_audioRows g_audioCallbacks g_audioABI g_audioDmaCount g_audioDma g_audioSavedADK g_audioActualADK
endif
.PHONY: verify-audio
verify: verify-audio
verify-audio:
	@python3 ../tools/audio_inputs.py --verify
endif
ifneq (,$(filter -DPOP2_UI,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/ui/UIInputs.h
.PHONY: verify-ui
verify: verify-ui
verify-ui:
	@python3 ../tools/ui_inputs.py --verify
endif
ifneq (,$(filter -DPOP2_DISPLAY,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/display/VideoTransfer.h
ifneq (,$(filter -DPOP2_DISPLAY_FRAMES_PROBE,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/display/DisplayFrames.h
endif
endif
ifneq (,$(findstring -DPOP2_FILE_REPLAY,$(EXTRA_DEFINES)))
obj/CoreRuntime.o: ../build/core/FileReplay.h
.PHONY: verify-file-replay
verify: verify-file-replay
verify-file-replay:
	@python3 ../tools/file_replay.py --verify
endif
endif
obj/DisplayC2P.o: ../src/platform/amiga/DisplayC2P.s ../src/platform/amiga/kalms/c2p1x1_8_c5_gen.s
	$(VASM) -m68020 -Felf -quiet -x -no-opt -nowarn=62 "$<" -o "$@"
obj/MacEntry.o: ../src/platform/amiga/MacEntry.s Makefile
	$(AS) -mcpu=68020 -g --register-prefix-optional $(if $(filter -DPOP2_AUDIO_ORDER_PROBE,$(EXTRA_DEFINES)),--defsym POP2_AUDIO_ORDER_PROBE=1) $(if $(filter -DPOP2_GAMEPLAY_ROUTE_PROBE,$(EXTRA_DEFINES)),--defsym POP2_GAMEPLAY_ROUTE_PROBE=1) $(if $(filter -DPOP2_GAMEPLAY_SAVE_PROBE,$(EXTRA_DEFINES)),--defsym POP2_GAMEPLAY_SAVE_PROBE=1) $(if $(filter -DPOP2_TIMING_TM_PROBE,$(EXTRA_DEFINES)),--defsym POP2_TIMING_TM_PROBE=1) $(if $(filter -DPOP2_TIMING_VBL_PROBE,$(EXTRA_DEFINES)),--defsym POP2_TIMING_VBL_PROBE=1) $(if $(filter -DPOP2_INTEGRATION_PROBE,$(EXTRA_DEFINES)),--defsym POP2_INTEGRATION_PROBE=1) "$<" -o "$@"
ifneq (,$(filter -DPOP2_TIMING_PROBE,$(EXTRA_DEFINES)))
PROBES += g_profileFreeFast g_profileLargestFast g_profileFreeChip g_c2pPasses g_c2pRects g_c2pPixels g_c2pUnbatched g_timingCount g_timingRows g_timingPhase g_timingSamples g_timingSetCount g_timingSetRows
endif
ifneq (,$(filter -DPOP2_OPENING_PROFILE,$(EXTRA_DEFINES)))
PROBES += g_openingCount g_openingPhases g_openingRows
endif
ifneq (,$(filter -DPOP2_TIMING_VBL_PROBE,$(EXTRA_DEFINES)))
PROBES += g_timingVblRecords g_timingVblResult g_timingVblElapsed g_timingVblUserCalls g_timingVblDeferredFields
endif
ifneq (,$(filter -DPOP2_TIMING_TM_PROBE,$(EXTRA_DEFINES)))
PROBES += g_timingTmRecords g_timingTmResult g_timingTmUserCalls
endif
ifeq ($(VERIFY),0)
ifeq ($(PROFILE),1)
PROBES := $(filter-out g_coreTrace g_coreCount,$(PROBES))
else
PROBES :=
endif
endif
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
	@POP2_PROFILE_BUILD=$(PROFILE) POP2_VERIFY_BUILD=$(VERIFY) python3 ../tools/check_scaffold_link.py $(OUT).elf obj/all.o $(PROBES)
	@python3 ../tools/build_receipt.py write '$(strip $(EXTRA_DEFINES) $(VERIFY_DEFINES))'
-include $(wildcard obj/*.d)

.PHONY: shared-runtime-check
shared-runtime-check:
	@cd .. && shasum -a 256 -c manifest/shared-runtime.sha256
$(OBJS): | shared-runtime-check
