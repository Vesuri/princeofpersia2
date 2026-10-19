#!/bin/bash
# Source in the same shell as builds and runs; AmigaXDev installs the shared tools.
. "${AMIGA_ENV:-$HOME/.local/share/amiga/env.sh}"
export AMIGA_PREFIX
export SUPPORT="${SUPPORT:-$AMIGA_PREFIX/support}"
export ELF2HUNK="${ELF2HUNK:-$AMIGA_SHARE/elf2hunk-fixed}"

# Explicit-clock validation uses the shared frequency and register corrections.
export FSUAE_CALIBRATED="${FSUAE_CALIBRATED:-$AMIGA_SHARE/fs-uae-register-observer/fs-uae}"
export FSUAE_SCOPED="${FSUAE_SCOPED:-$AMIGA_SHARE/fs-uae-native-service-scope/fs-uae}"

# Kalms upstream includes the Exec LVO definitions; ordinary init does not call them.
export NDK_ASM="${NDK_ASM:-$AMIGA_SHARE/ndk/Include_I}"
