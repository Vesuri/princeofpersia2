#!/bin/bash
# Run a bounded read-only GDB script; defaults to the scaffold restoration probe.
set -euo pipefail
if [ -n "${1:-}" ]; then export GDBSCRIPT="$1"; fi
export WINDOW="${WINDOW:-back}"
exec /bin/bash "$(dirname "$0")/run.sh"
