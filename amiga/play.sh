#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir .run-lock
RUN="$(mktemp -d .run-play-XXXXXX)"
FSUAE_RUN="$RUN"
cleanup() {
  if type fsuae_stop >/dev/null 2>&1; then fsuae_stop; fi
  rmdir .run-lock
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
. ./fsuae.sh
python3 - <<'PYCODE'
import sys
sys.path.insert(0,'../tools')
from build_receipt import verify
expected={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI','-DPOP2_AUDIO'}
if set(verify()['flags'].split())!=expected:
    raise SystemExit('Play requires a release build. Run make release first.')
PYCODE
python3 ../tools/scaffold_inputs.py calibrated
FSUAE="$FSUAE_CALIBRATED"
AMIGA_SETUP="${AMIGA_SETUP:-a1200-030}"
NTSC=0 DEBUG=0 SOUND="${SOUND:-1}" WINDOW="${WINDOW:-front}" WARP="${WARP:-0}"
DH0="$RUN/dh0"; DH1="$RUN/dh1"
mkdir -p "$DH0/s" "$DH0/c" "$DH1" "$RUN/state" "$RUN/logs"
cp out/PrinceOfPersia2.exe "$DH1/PrinceOfPersia2"
cp "$SETPATCH" "$DH0/c/SetPatch"
python3 ../tools/stage_core.py "$DH1"
cat > "$DH0/s/startup-sequence" <<'STARTUP'
C:SetPatch QUIET
FailAt 21
Stack 16384
cd dh1:
PrinceOfPersia2
If WARN
Echo "SHELL FAIL" >Shell.done
Else
Echo "SHELL PASS" >Shell.done
EndIf
STARTUP
printf '%s\n' "$RUN" > .run-play-last.txt
fsuae_options
fsuae_launch --logs_dir="$PWD/$RUN/logs" --hard_drive_0="$DH0" --hard_drive_1="$DH1" --state_dir="$RUN/state" --joystick_port_0=nothing --joystick_port_1=nothing
printf '%s\n' "$FSUAE_PID" > "$RUN/interactive.pid"
wait "$FSUAE_PID"
