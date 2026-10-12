# Source only from this project's launcher, after selecting a fresh run directory.
. ./env.sh
python3 ../tools/scaffold_inputs.py run || return 1
DEBUG_PORT="${DEBUG_PORT:-24379}"
export DEBUG_PORT
FSUAE_CLAIM_STRICT=1
. "$FSUAE_COMMON"
