SHELL := /bin/bash
.DEFAULT_GOAL := help

.PHONY: help verify-input check inspect extract-original-data check-extraction
help:
	@echo 'Original archive inspection and extraction; no game build yet.'
	@echo 'make verify-input  verify the locally staged original archive'
	@echo 'make inspect       reproduce the source inventory from the archive'
	@echo 'make extract-original-data  verify and extract to tmp/runtime-data/'
	@echo 'make check-extraction  repeat extraction and run rejection/preservation controls'
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
