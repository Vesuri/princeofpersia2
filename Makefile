SHELL := /bin/bash
.DEFAULT_GOAL := all

.PHONY: help verify-input check inspect extract-original-data check-extraction reference check-reference
help:
	@echo 'make / make host-tests  build the empty host program and test portable helpers'
	@echo 'make amiga / make scaffold-check  build / verify the empty Amiga boot and quit'
	@echo 'make core / make core-check  build / verify bounded original startup'
	@echo 'make display / make display-check  build / verify display services and AGA presentation'
	@echo 'make audio / make audio-check  build / verify original audio calls and Paula DMA'
	@echo 'make ui / make ui-check  build / verify VBI input and port-owned UI'
	@echo 'make files-check   verify paired file/resource calls and save persistence'
	@echo 'Original archive tools, Amiga scaffold and startup runtime.'
	@echo 'make verify-input  verify the locally staged original archive'
	@echo 'make inspect       reproduce the source inventory from the archive'
	@echo 'make extract-original-data  verify and extract to tmp/runtime-data/'
	@echo 'make check-extraction  repeat extraction and run rejection/preservation controls'
	@echo 'make reference     run the pinned headless Macintosh opening checkpoint'
	@echo 'make check-reference  repeat the reference and run rejection controls'
	@echo 'make map           generate the flow-following static map'
	@echo 'make ghidra        export the pinned headless Ghidra analysis'
	@echo 'make static-map-check  verify two live maps and rejection controls'
	@echo 'make check         verify the archive and shared compiler access'

verify-input:
	@shasum -a 256 -c manifest/originals.sha256

check: verify-input
	@. amiga/env.sh && m68k-amiga-elf-gcc --version

inspect: verify-input
	@. amiga/env.sh && python3 tools/inspect_original.py

extract-original-data: verify-input
	@. amiga/env.sh && python3 tools/extract_original_data.py

check-extraction: verify-input
	@. amiga/env.sh && python3 tools/check_extraction.py

reference:
	@. amiga/env.sh && python3 tools/run_reference.py

check-reference:
	@. amiga/env.sh && python3 tools/check_reference.py

.PHONY: map ghidra static-map-check
map:
	@. amiga/env.sh && python3 tools/static_map.py

ghidra:
	@. amiga/env.sh && python3 tools/run_ghidra.py

static-map-check: ghidra
	@. amiga/env.sh && python3 tools/test_static_map.py
	@. amiga/env.sh && python3 tools/check_map_gate.py

.PHONY: all host-tests amiga scaffold-check slave dist-scaffold
all: host-tests
host-tests:
	@. amiga/env.sh && python3 tools/host_tests.py
amiga:
	@. amiga/env.sh && $(MAKE) -C amiga
scaffold-check: host-tests amiga
	@. amiga/env.sh && python3 tools/check_scaffold.py
slave:
	@. amiga/env.sh && $(MAKE) -C whdload
dist-scaffold: amiga slave
	@. amiga/env.sh && python3 tools/package_release.py

.PHONY: core-inputs core
core-inputs:
	@. amiga/env.sh && python3 tools/core_inputs.py
core: core-inputs
	@. amiga/env.sh && $(MAKE) -C amiga EXTRA_DEFINES=-DPOP2_CORE
.PHONY: core-check
core-check: host-tests
	@. amiga/env.sh && python3 tools/check_core.py

.PHONY: files-check
files-check: host-tests
	@. amiga/env.sh && python3 tools/check_files.py

.PHONY: display-inputs display display-check
display-inputs: core-inputs
	@. amiga/env.sh && python3 tools/display_inputs.py
display: display-inputs
	@. amiga/env.sh && $(MAKE) -C amiga 'EXTRA_DEFINES=-DPOP2_CORE -DPOP2_DISPLAY'
display-check: host-tests
	@. amiga/env.sh && python3 tools/check_display.py

.PHONY: ui-inputs ui
ui-inputs: display-inputs
	@. amiga/env.sh && python3 tools/ui_inputs.py
ui: ui-inputs
	@. amiga/env.sh && $(MAKE) -C amiga 'EXTRA_DEFINES=-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI'

.PHONY: ui-check
ui-check: host-tests ui-inputs
	@. amiga/env.sh && python3 tools/check_ui.py

.PHONY: audio-inputs audio
audio-inputs: ui-inputs
	@. amiga/env.sh && python3 tools/audio_inputs.py
audio: audio-inputs
	@. amiga/env.sh && $(MAKE) -C amiga 'EXTRA_DEFINES=-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO'

.PHONY: audio-check
audio-check: host-tests audio-inputs
	@. amiga/env.sh && python3 tools/check_audio.py
