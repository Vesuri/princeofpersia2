#!/bin/bash
# Source in the same shell as builds and runs; AmigaXDev installs the shared tools.
. "${AMIGA_ENV:-$HOME/.local/share/amiga/env.sh}"
export AMIGA_PREFIX
export SUPPORT="${SUPPORT:-$AMIGA_PREFIX/support}"
export ELF2HUNK="${ELF2HUNK:-$AMIGA_SHARE/elf2hunk-fixed}"
