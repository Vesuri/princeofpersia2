#!/bin/bash
# Same launch/staging path as run.sh, with a caller-selected read-only observer.
set -euo pipefail
export DEADLINE="${1:-60}"
exec /bin/bash "$(dirname "$0")/run.sh"
