# KickEmu release launcher.
WHDLOAD ?= $(HOME)/.local/share/amiga/WHDLoad
NDK ?= $(HOME)/.local/opt/m68k-amiga-elf/sys-include
VASM ?= $(HOME)/.local/vasmm68k_mot
OUT = ../build/whdload
.PHONY: all FORCE
all: $(OUT)/PrinceOfPersia2.slave
$(OUT)/PrinceOfPersia2.slave: PrinceSlave.s PatchBinding.s ../VERSION FORCE
	@python3 ../tools/scaffold_inputs.py build
	@python3 ../tools/scaffold_inputs.py slave
	@cd .. && shasum -a 256 -c manifest/install-shared.sha256
	@mkdir -p $(OUT)/include/lvo
	@for lib in dos exec graphics expansion; do ln -sf "$(NDK)/lvo/$${lib}_lib.i" "$(OUT)/include/lvo/$$lib.i"; done
	$(VASM) $(EXTRA_FLAGS) -m68020 -pic -x -devpac -Fhunkexe -nosym -I$(OUT)/include -I"$(WHDLOAD)/Include" -I"$(WHDLOAD)/Src/sources" -I"$(NDK)" -o $@ $<
