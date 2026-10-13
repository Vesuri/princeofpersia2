SHELL := /bin/bash
.DEFAULT_GOAL := all

.PHONY: help verify-input check inspect extract-original-data check-extraction reference check-reference
help:
	@echo 'make / make release  build the game without runtime verification; VERIFY=1 opts in'
	@echo 'make play AMIGA_SETUP=a1200-040  build release and play; make scaffold-check tests empty boot'
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
all: release
host-tests: shared-runtime-check
	@. amiga/env.sh && python3 tools/host_tests.py
amiga: release
.PHONY: release scaffold
release: audio-inputs
	@. amiga/env.sh && $(MAKE) -C amiga
scaffold:
	@. amiga/env.sh && $(MAKE) -C amiga VERIFY=1 EXTRA_DEFINES=''
scaffold-check: host-tests scaffold
	@. amiga/env.sh && python3 tools/check_scaffold.py
slave:
	@. amiga/env.sh && $(MAKE) -C whdload
dist-scaffold: scaffold slave
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

.PHONY: shared-runtime-check
shared-runtime-check:
	@shasum -a 256 -c manifest/shared-runtime.sha256

.PHONY: integration-check integration-contracts-check
integration-contracts-check: shared-runtime-check
	@. amiga/env.sh && python3 tools/check_integration_contracts.py

integration-check: host-tests audio-inputs integration-contracts-check
	@. amiga/env.sh && python3 tools/check_integration.py

.PHONY: play
play: release
	@/bin/bash amiga/play.sh

.PHONY: gameplay-restart-check
gameplay-restart-check: host-tests audio-inputs
	@. amiga/env.sh && python3 tools/check_gameplay.py

.PHONY: gameplay-save-check
gameplay-save-check: host-tests audio-inputs
	@. amiga/env.sh && python3 tools/check_gameplay.py --route save-load

.PHONY: gameplay-import-check
gameplay-import-check: host-tests audio-inputs
	@. amiga/env.sh && python3 tools/check_gameplay.py --route load-import
