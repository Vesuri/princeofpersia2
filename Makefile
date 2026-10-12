SHELL := /bin/bash
.DEFAULT_GOAL := help

.PHONY: help verify-input check
help:
	@echo 'Initial setup only; no game build or analysis targets yet.'
	@echo 'make verify-input  verify the locally staged original archive'
	@echo 'make check         verify the archive and shared compiler access'

verify-input:
	@shasum -a 256 -c manifest/originals.sha256

check: verify-input
	@. amiga/env.sh && m68k-amiga-elf-gcc --version
