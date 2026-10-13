# Timing

Item 11 is complete for the owner-selected `a1200-030` target. The original
five-tick gameplay cadence passes, including the first step; Time Manager and
VBL service fixtures pass too. The owner deferred 68040 validation to release
item 14 and 68020 work to final item 15. Neither CPU is covered by this completion.

## Clock and callback services

TickCount uses shared `VideoTicks` arithmetic to interpolate 60 Hz ticks within
50 Hz PAL fields. A field-only 6/5 accumulator skips one tick in each six-tick
group and can turn a five-tick wait into six. Host tests cover every microsecond
over 100 fields and tick wrap. The native sampler rejects scanline zero before
the VBI epoch catches up and ambiguous horizontal positions 0/1; a backward
clock remains a named failure.

Time Manager and VBL callbacks run at safe user-mode service boundaries with
register and condition-code preservation. The shared AmigaXDev `VBLPasses` helper
retains every elapsed queue pass, as in Alone in the Dark, rather than collapsing
several elapsed ticks into one callback. Task ownership and countdown policy
remain in the port adapter. An empty VBL queue needs no beam sample; installation
and final removal establish/reset its epoch. Audio polling uses the existing
pending-work predicate. Original game timers and logic are unchanged.

## Original-paired service fixtures

`tools/mac_timing.lua` runs authored VBL records after the verified Macintosh
opening. Initial counts 1 and 2 each call back three times, rearming with count 1,
then leave zero and are automatically removed. Initial counts 0 and -1 decrement
modulo 16 bits without calling back during thirty ticks. `VRemove` returns -1 for
the already removed tasks and succeeds for the dormant tasks. These are MEASURED
System 7.5.5 contracts, pinned in `manifest/timing-vbl-reference.sha256`.

The native fixture additionally defers a task for six PAL fields, then requires
three callbacks with consecutive virtual ticks at the next safe point. It checks
user-mode execution, register/condition-code canaries, unchanged original CODE,
heap ownership and restored hardware. The 68030 fixture passes its thirty-tick
contract and twelve altered-evidence rejection controls.

`tools/mac_time_manager.lua` measures seven cases: 40 milliseconds, -40,000
microseconds, an 80 ms task primed again with 1 ms, a 20 ms task primed again with
100 ms, cancellation of a 200 ms task, zero delay, and three callbacks rearming
at 20 ms. Cancellation retains a negative microsecond remainder until a second
removal clears it. Two completed original runs reproduce the contract pinned in
`manifest/timing-tm-reference.sha256`. The 68030 fixture matches it, including
user-mode ABI, CODE/heap audits, restoration and twelve rejection controls.
TickCount resolves 1/60 second; deadline bounds allow either clock phase and do
not establish microsecond-resolution accuracy.

Reproduce the VBL fixture explicitly:

```sh
. amiga/env.sh
make -C amiga VERIFY=1 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_VBL_PROBE'
AMIGA_SETUP=a1200-030 CORE=1 TIMING_VBL=1 \
  TIMING_VBL_REFERENCE="$PWD/ref/mame/runs/timing-vbl-2" \
  GDBSCRIPT="$PWD/amiga/timing_vbl.gdb" DEADLINE=1200 bash amiga/run.sh
```

For Time Manager substitute `POP2_TIMING_TM_PROBE`, `TIMING_TM=1`,
`TIMING_TM_REFERENCE`, `timing-tm-3` and `amiga/timing_tm.gdb`. The respective
Python checker accepts native and original directories, `--setup a1200-030`, and
`--controls`. Generate fresh original fixtures with `POP2_TIMING_VBL=1` or
`POP2_TIMING_TM=1` and `tools/run_reference.py`; reference input pins must match.

## Gameplay cadence

Two completed original routes reproduce 100 timer-1 requests of five ticks,
with all 99 intervals exactly five ticks. The normalized contract is pinned in
`manifest/timing-cadence-reference.sha256`; absolute epochs and addresses are
excluded. `tools/check_timing_cadence.py` requires positive route completion,
reference initial state, matching converted bitplanes, consecutive original frame
counters and every timer interval, including the first. Six alterations exercise
its rejection checks; a synthetic positive is only a checker control.

MEASURED: the final 68030 route has 59 consecutive five-tick intervals. Its
five-update window takes 20 PAL fields (80 ms/update including pacing), with the
same endpoint screen/context and conversion work as the pinned parent. All 59
route updates convert once at frame end. The benchmark and death screen/context
bytes match the parent, and both captured planar images match the chunky output.

The first update previously inherited a nearly full-screen synchronization copy
from the initial scene. The existing standalone DrawWorld boundary now completes
that scene's large buffer synchronization before returning to game execution.
After the VBI swap, the old front is inactive and the shared blitter can safely
copy into it. FrameAdv updates and ordinary full-screen animation retain deferred
synchronization. This schedules scene-load work earlier; it is not a claim of
reduced total copying or faster scene loading. Original timer requests are intact.

The 68040 preset remains below the cadence gate: the measured scene-copy
experiment has 48 five-tick intervals and eight intervals of six to eight ticks.
Its five-update endpoint matches, but its death image differs from the 68030
parent, so the complete parent-route comparison fails. This is an emulator
measurement, not a prediction of relative hardware CPU speeds, and is not an
accepted faster-CPU result.

Use the profiling route in [performance measurements](performance.md), then:

```sh
python3 tools/check_timing_cadence.py NATIVE_RUN ORIGINAL_REFERENCE \
  --setup a1200-030 --controls
```

The final display fixture independently decodes four complete frames, checking
scene preparation, dirty preservation, palette-only updates, viewport movement,
field pointers and system restoration. Its checker accepts `--setup a1200-030`;
scaffold and gameplay fixtures share the effective-machine checks. Three recorded
presets pass and six wrong-preset pairings are rejected.

## Measurement boundary

`PROFILE=1 VERIFY=0` retains explicit input, timing records and phase samples
without runtime hashing, trap tracing or expensive exit audits. Ordinary release
has neither profiling nor verification probes; link audits enforce this.
`VERIFY=1` is required for correctness fixtures. Restore ordinary play with
`make release` after diagnostics. Captures remain ignored.

These bounded routes do not establish all-level cadence, full-game coverage,
title phase pairing or real-hardware performance. Those remain separate
validation and release work.
