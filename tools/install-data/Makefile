.DEFAULT_GOAL := all
# Run from anywhere: make -C tools/install-data [amiga]
# Source amiga/env.sh before building the Amiga binary.
ROOT := ../..
BUILD ?= $(ROOT)/build/install-data
CC ?= cc
CFLAGS ?= -O2 -g
WARN = -std=c99 -Wall -Wextra -Werror
SOURCES = install.c sha256.c
HEADERS = io.h sha256.h sit13_tables.h originals.h arsenic.h decode.inc support.inc support.h install_config.h
AMIGA_CC = m68k-amiga-elf-gcc
ELF2HUNK ?= elf2hunk
AMIGA_NM = $(dir $(shell command -v $(AMIGA_CC)))../m68k-amiga-elf/bin/nm
AMIGA_FLAGS = -std=c99 -O2 -g -m68020 -mtune=68020 -msoft-float -nostdlib -fno-builtin -fno-tree-loop-distribute-patterns -fstack-usage -Wall -Wextra -Wno-volatile-register-var -DINSTALL_AMIGA
AMIGA_FLAGS += $(EXTRA_AMIGA_FLAGS)
.PHONY: all amiga test shared-check
shared-check:
	@cd $(ROOT) && shasum -a 256 -c manifest/install-shared.sha256
$(BUILD)/Pop2InstallData $(BUILD)/install.o $(BUILD)/io_amiga.o $(BUILD)/sha256.o $(BUILD)/start.o: | shared-check
all: $(BUILD)/Pop2InstallData
$(BUILD):
	mkdir -p $@
$(BUILD)/Pop2InstallData: $(SOURCES) io_unix.c $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(WARN) $(SOURCES) io_unix.c -o $@
amiga: toolchain-check $(BUILD)/Pop2InstallData.exe
.PHONY: toolchain-check
toolchain-check:
	@cd $(ROOT) && python3 tools/scaffold_inputs.py build
$(BUILD)/install.o: install.c $(HEADERS) Makefile | $(BUILD)
	$(AMIGA_CC) $(AMIGA_FLAGS) -c $< -o $@
$(BUILD)/sha256.o: sha256.c sha256.h io.h Makefile | $(BUILD)
	$(AMIGA_CC) $(AMIGA_FLAGS) -c $< -o $@
$(BUILD)/io_amiga.o: io_amiga.c io.h Makefile | $(BUILD)
	$(AMIGA_CC) $(AMIGA_FLAGS) -c $< -o $@
$(BUILD)/start.o: start.s | $(BUILD)
	$(AMIGA_CC) -m68020 -mtune=68020 -c $< -o $@
$(BUILD)/Pop2InstallData.elf: $(BUILD)/start.o $(BUILD)/install.o $(BUILD)/sha256.o $(BUILD)/io_amiga.o
	$(AMIGA_CC) -nostdlib -m68020 -mtune=68020 -Wl,--emit-relocs,-Ttext=0,-e,_start,-Map=$(BUILD)/Pop2InstallData.map $^ -o $@
	$(AMIGA_NM) -u $@ > $(BUILD)/undefined.txt
	@test ! -s $(BUILD)/undefined.txt
$(BUILD)/Pop2InstallData.exe: $(BUILD)/Pop2InstallData.elf $(ELF2HUNK)
	"$(ELF2HUNK)" $< $@ -s
test: all
	python3 test_install.py $(BUILD)/Pop2InstallData
