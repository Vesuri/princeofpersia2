#!/bin/bash
# Opening-sequence profile (item 19, docs/performance.md): no-input route from
# the first room to the Kid's death song, with one exact-cycle profile per phase
# (ended by the Kid's landing and each health change). Builds the PROFILE=1
# VERIFY=0 opening executable first.
#   aprof.sh NAME [SETUP] [DEADLINE]
# SETUP is a1200-030 (default) or a1200-fast. The cycle profiler refuses an MMU
# model, so a1200-030 is profiled without one; APROF_CYCLES=0 instead runs the
# ordinary emulator on the unmodified preset to record the matched field cadence
# (APROF_NO_MMU=1 adds the no-MMU model as an emulator control).
# Writes ../tmp/aprof/NAME/; summarize with python3 ../tools/aprof_report.py opening NAME.
set -euo pipefail
cd "$(dirname "$0")"
. ./env.sh
usage() { echo 'usage: aprof.sh NAME [a1200-030|a1200-fast] [DEADLINE]' >&2; exit 2; }
[[ $# -ge 1 && $# -le 3 ]] || usage
name=$1 setup=${2:-a1200-030} deadline=${3:-3600} cycles=${APROF_CYCLES:-1}
[[ "$name" =~ ^[a-z0-9][a-z0-9_-]*$ ]] || { echo 'APROF / INVALID NAME' >&2; exit 2; }
[[ "$setup" == a1200-030 || "$setup" == a1200-fast ]] || usage
[[ "$deadline" =~ ^[1-9][0-9]*$ && ( "$cycles" == 0 || "$cycles" == 1 ) ]] || usage
out=../tmp/aprof/$name
# Do not silently replace a previous measurement.
[[ ! -e "$out" ]] || { echo "APROF / NAME EXISTS: $name" >&2; exit 1; }
mkdir -p "$out"
make -j4 PROFILE=1 VERIFY=0 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_PROBE -DPOP2_INTEGRATION_NO_RIGHT -DPOP2_OPENING_PROFILE' \
  >"$out/build.log" 2>&1 || { echo "APROF / BUILD FAILED: $out/build.log" >&2; exit 1; }
nommu=${APROF_NO_MMU:-$cycles}
[[ "$setup" == a1200-030 || "$nommu" == 0 ]] || nommu=0
[[ "$nommu" == 0 || "$nommu" == 1 ]] || usage
# The profiler requires the no-MMU model; APROF_NO_MMU=1 with APROF_CYCLES=0 is the control.
[[ "$cycles" == 0 || "$setup" != a1200-030 || "$nommu" == 1 ]] || usage
# FS-UAE deterministic mode (used by Alone in the Dark) fails GLib queue
# assertions here and never serves the debugger stub with either emulator, so
# reproducibility is established by repeat runs (aprof_report.py --compare).
status=0
AMIGA_SETUP="$setup" CORE=1 OPENING_PROFILE=1 CPU_PROFILE="$cycles" \
  PROFILE_NO_MMU="$nommu" GDBSCRIPT="$PWD/aprof_opening.gdb" DEADLINE="$deadline" \
  bash run.sh >"$out/run.log" 2>&1 || status=$?
run=$(cat .run-last.txt)
printf "%s\n" "$run" >"$out/run-dir.txt"
printf 'setup=%s cycles=%s no_mmu=%s\n' "$setup" "$cycles" "$nommu" >"$out/config.txt"
for f in gdb-out.log PrinceOfPersia2.elf build-receipt.json build.sha256 jt.bin opening-frames.bin timing-frames.bin timing-set.bin \
         first-room-screen.bin first-room-colors.bin death-screen.bin; do
  [[ ! -e "$run/$f" ]] || cp "$run/$f" "$out/"
done
cp "$run"/opening-*.prof "$out/" 2>/dev/null || true
cp "$run/fsuae/"*.uae "$out/" 2>/dev/null || true
grep -E '^(INTEGRATION STAGE|OPENING|PROFILE (START|STOP)|CORE REASON)' "$out/gdb-out.log" || true
if [[ "$status" != 0 ]]; then
  echo "APROF / INCOMPLETE (status $status): $out/run.log" >&2
  exit 1
fi
echo "APROF COMPLETE $out"
