#!/bin/bash
# Bounded scaffold/core run. Fresh private disks, frozen executable and ELF.
set -euo pipefail
CALLER="$PWD"
SCRIPT="${GDBSCRIPT:-}"
if [ -n "$SCRIPT" ] && [[ "$SCRIPT" != /* ]]; then SCRIPT="$CALLER/$SCRIPT"; fi
cd "$(dirname "$0")"
if ! mkdir .run-lock 2>/dev/null; then echo 'SCAFFOLD / RUN ALREADY ACTIVE'; exit 1; fi
RUN="$(mktemp -d .run-XXXXXX)"
FSUAE_RUN="$RUN"
GDB_PID=
cleanup() {
  if [ -n "$GDB_PID" ]; then kill -9 "$GDB_PID" 2>/dev/null || true; wait "$GDB_PID" 2>/dev/null || true; fi
  if type fsuae_stop >/dev/null 2>&1; then fsuae_stop; fi
  rmdir .run-lock
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
. ./fsuae.sh
python3 ../tools/build_receipt.py verify
FSUAE="$FSUAE_ARM"
AMIGA_SETUP="${AMIGA_SETUP:-a1200-fast}"
NTSC=0 DEBUG=1 SOUND="${SOUND:-0}" WINDOW="${WINDOW:-none}" WARP="${WARP:-1}"
DEADLINE="${DEADLINE:-60}"
[[ "$DEADLINE" =~ ^[1-9][0-9]*$ ]] || exit 2
SNAPSHOT_AFTER="${SNAPSHOT_AFTER:-0}"
[[ "$SNAPSHOT_AFTER" =~ ^[0-9]+$ ]] || exit 2
DH0="$RUN/dh0"; DH1="$RUN/dh1"
mkdir -p "$DH0/s" "$DH0/c" "$DH1" "$RUN/state" "$RUN/logs" "$RUN/gdbhome"
cp out/PrinceOfPersia2.exe "$DH1/PrinceOfPersia2"
cp out/PrinceOfPersia2.elf "$RUN/PrinceOfPersia2.elf"
cp "$SETPATCH" "$DH0/c/SetPatch"
if [ "${CORE:-0}" = 1 ]; then
  python3 ../tools/stage_core.py "$DH1"
  sed "s|@RUN@|$RUN|g" core.gdb > "$RUN/core.gdb"
  if [ -z "$SCRIPT" ]; then SCRIPT="$RUN/core.gdb"; fi
fi
if [ -n "$SCRIPT" ]; then
  sed "s|@RUN@|$RUN|g" "$SCRIPT" > "$RUN/observer.gdb"
  SCRIPT="$RUN/observer.gdb"
fi
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
printf '%s\n' "$RUN" > .run-last.txt
shasum -a 256 "$DH1/PrinceOfPersia2" "$RUN/PrinceOfPersia2.elf" > "$RUN/build.sha256"
fsuae_claim_port
fsuae_options
fsuae_launch --logs_dir="$PWD/$RUN/logs" --hard_drive_0="$DH0" --hard_drive_1="$DH1" --remote_debugger_trigger=PrinceOfPersia2 --state_dir="$RUN/state" --joystick_port_0=nothing --joystick_port_1=nothing
for ((i=0;i<60;i++)); do
 kill -0 "$FSUAE_PID" 2>/dev/null || { echo 'SCAFFOLD / EMULATOR EXIT'; exit 1; }
 lsof -nP -iTCP:"$DEBUG_PORT" -sTCP:LISTEN >/dev/null 2>&1 && break
 sleep 1
done
lsof -nP -iTCP:"$DEBUG_PORT" -sTCP:LISTEN >/dev/null 2>&1 || { echo 'SCAFFOLD / STUB TIMEOUT'; exit 124; }
printf 'set pagination off\nset confirm off\nset remotetimeout 20\ntarget remote 127.0.0.1:%s\n' "$DEBUG_PORT" > "$RUN/connect.gdb"
env HOME="$PWD/$RUN/gdbhome" XDG_CACHE_HOME="$PWD/$RUN/gdbhome" m68k-amiga-elf-gdb --batch -q -l 10 -x "$RUN/connect.gdb" -x "${SCRIPT:-scaffold.gdb}" "$RUN/PrinceOfPersia2.elf" > "$RUN/gdb-out.log" 2>&1 &
GDB_PID=$!
for ((i=0;i<DEADLINE;i++)); do
 kill -0 "$GDB_PID" 2>/dev/null || break
 # Diagnostic observers report a stopped CPU; this is never a completion gate.
 if [ "$SNAPSHOT_AFTER" -gt 0 ] && [ "$i" -eq "$SNAPSHOT_AFTER" ]; then kill -INT "$GDB_PID"; fi
 sleep 1
done
if kill -0 "$GDB_PID" 2>/dev/null; then echo 'SCAFFOLD / PROBE TIMEOUT'; exit 124; fi
status=0;wait "$GDB_PID" || status=$?;GDB_PID=
cat "$RUN/gdb-out.log"
[ "$status" = 0 ] || exit "$status"
for ((i=0;i<20;i++)); do [ -f "$DH1/Shell.done" ] && break; sleep 1; done
if [ "${CORE:-0}" = 1 ]; then
  if [ "${RELEASE_STARTUP:-0}" = 1 ]; then
    python3 ../tools/check_release_startup.py "$RUN" "$RELEASE_REFERENCE"
  elif [ "${INTEGRATION:-0}" = 1 ]; then
    if [ "${INTEGRATION_NO_RIGHT:-0}" = 1 ]; then
      python3 ../tools/check_integration_result.py "$RUN" "$INTEGRATION_REFERENCE" "$INTEGRATION_TITLE_REFERENCE" --no-right
    else
      python3 ../tools/check_integration_result.py "$RUN" "$INTEGRATION_REFERENCE" "$INTEGRATION_TITLE_REFERENCE"
    fi
  elif [ "${AUDIO_PROBE:-0}" = 1 ]; then
    python3 ../tools/check_audio_result.py "$RUN"
  elif [ "${AUDIO_STARTUP:-0}" = 1 ]; then
    python3 ../tools/check_audio_result.py "$RUN" --startup "$AUDIO_REFERENCE"
  elif [ "${UI_PROBE:-0}" = 1 ]; then
    python3 ../tools/check_ui_result.py "$RUN" "$UI_REFERENCE"
  elif [ "${UI_STARTUP:-0}" = 1 ]; then
    python3 ../tools/check_ui_result.py "$RUN" "$UI_REFERENCE" --startup
  elif [ "${INPUT_PROBE:-0}" = 1 ]; then
    python3 ../tools/check_ui_result.py "$RUN" "$INPUT_REFERENCE" --input
  elif [ "${DISPLAY_STARTUP:-0}" = 1 ]; then
    python3 ../tools/check_display_startup.py "$RUN" "$DISPLAY_REFERENCE"
  elif [ "${DISPLAY_SCENES:-0}" = 1 ]; then
    python3 ../tools/check_display_scenes.py "$RUN" "$DISPLAY_REFERENCE"
  elif [ "${DISPLAY_SERVICES:-0}" = 1 ]; then
    python3 ../tools/check_display_services.py "$RUN" "$DISPLAY_REFERENCE"
  else
    python3 ../tools/check_core_result.py "$RUN"
  fi
else
  python3 ../tools/scaffold_result.py "$RUN"
fi
