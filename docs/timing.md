# Timing

Item 11 remains open: full-game step cadence has not matched the Macintosh
reference on both required CPU configurations. The service fixture below verifies
VBL semantics independently of that acceptance gate.

## VBL service fixture

`tools/mac_timing.lua` runs authored task records after the verified Macintosh
opening. Initial counts 1 and 2 each call back three times, rearming with count 1,
then leave count zero. Initial counts 0 and -1 decrement modulo 16 bits without
calling back during thirty ticks. `VRemove` subsequently reports -1 for the
already removed positive-count tasks and succeeds for the dormant tasks.
These are MEASURED System 7.5.5 contracts. No original game logic is substituted.

The native fixture adds register/condition-code checks and asserts that its
service calls originate in user mode. A fifth task is installed dormant, armed,
and held away from service boundaries for six PAL fields. At the next safe point,
three callbacks must observe consecutive virtual ticks before automatic removal.
The shared AmigaXDev `VBLPasses` helper retains each elapsed queue pass; task
countdown and ownership policies remain in the port adapter.

The field-only clock implementation passed on `a1200-fast` (fixed 68020)
and the cached `a1200-040` preset, with restored
hardware, heap and exception vectors and unchanged original CODE. Its checker
rejects missing completion, missing callbacks, failed ABI checks, collapsed
callback ticks, wrong removal results and truncated records. These checks do not
establish full gameplay cadence or real-hardware timing.

For an original capture, source `amiga/env.sh` and run:

```sh
POP2_TIMING_VBL=1 python3 tools/run_reference.py --name timing-vbl-reference
```

The reference directory must pass the existing original-input checks. The VBL
checker pins normalized observed task results through
`manifest/timing-vbl-reference.sha256`, excluding relocated pointers, absolute
clock epochs and unrelated emulator messages.

Build and run the native fixture explicitly (ordinary builds remain release):

```sh
. amiga/env.sh
make -C amiga VERIFY=1 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_VBL_PROBE'
AMIGA_SETUP=a1200-fast CORE=1 TIMING_VBL=1 \
  TIMING_VBL_REFERENCE="$PWD/ref/mame/runs/timing-vbl-reference" \
  GDBSCRIPT="$PWD/amiga/timing_vbl.gdb" DEADLINE=1200 /bin/bash amiga/run.sh
```

Repeat with `AMIGA_SETUP=a1200-040`. The launcher checks the effective emulator
configuration, not only the requested profile. Run the checker with `--controls`
to exercise evidence rejection; its positional arguments are the native run and
original reference directories, followed by `--setup` with the selected profile.
Restore ordinary play with `make release` after diagnostics. Captures and task
records stay in ignored reference/run directories.

## Time Manager service fixture

`tools/mac_time_manager.lua` measures seven cases after the original opening:
40 milliseconds, -40,000 microseconds, an 80 ms task primed again with 1 ms,
a 20 ms task primed again with 100 ms, cancellation of a 200 ms task, zero delay,
and three callbacks rearming at 20 ms. Two completed original runs reproduce the
observed counts, status flags, return values and tick intervals. Cancellation
retains a negative microsecond remainder until a second removal clears it.

The native counterpart checks the same service results and deadlines, with
register/condition-code canaries and user-mode callback checks. TickCount only
resolves 1/60 second: deadline bounds allow either clock phase and do not claim
microsecond-resolution accuracy. The field-only clock implementation passed on fixed 68020 and cached 68040, along with eleven
evidence rejection controls. The cached 68040 run also verifies the dispatcher
limited to the occupied task extent. Neither fixture establishes full-game
step cadence. Machine checks require the effective data-cache setting.

Generate an original capture with `POP2_TIMING_TM=1` instead of
`POP2_TIMING_VBL=1`. Build the native fixture with `POP2_TIMING_TM_PROBE` instead
of `POP2_TIMING_VBL_PROBE`, and launch with `TIMING_TM=1`,
`TIMING_TM_REFERENCE` and `amiga/timing_tm.gdb`. The original record contract is
pinned in `manifest/timing-tm-reference.sha256`; absolute addresses and clock
epochs are excluded. `tools/check_timing_tm.py` accepts the native directory,
original directory, `--setup` and optional `--controls`, like the VBL checker.

## Gameplay cadence investigation

The diagnostic recorder confirms the PAL-field-to-TickCount ratio on both tested
CPU configurations. Original gameplay requests five-tick steps; the fixed 68020
still overruns those deadlines. The service fixtures alone do not satisfy item 11.

Instruction profiling identified resource searches on already-recognized service
veneers. Caller classification now resolves those veneers before searching for
the audio resource, retaining the existing ownership and original-byte checks.
The resulting fixed-68020 route passes the title/first-room reference pixels,
state, gameplay input, code audit and restoration checks. Timer dispatch also uses
the occupied task extent already maintained by installation and removal.

The general service bridge now operates on the parked register image, as Alone
in the Dark and this port's callback path do. Overlapping argument cleanup uses
`memmove`; unchanged stack positions need no register copy. A fixed-68020
comparison covers five updates after the initial-room checkpoint: no Right input
was consumed, CODE 2–6 and 17 instruction counts match, and the complete captured
end-context bytes match. The diagnostic interval changes from 117 to 114 PAL
fields (468 to 456 ms per update). CODE 11 music callbacks follow elapsed time
and consequently have different counts. This remains far above the original's
five-tick step requests; it does not satisfy the cadence gate.

These are instrumented emulation measurements, not release-build throughput or
real-hardware claims. The initial scene is counted before the profiling checkpoint;
ending at row six therefore covers five subsequent updates. Earlier profiles with
timed input at different frame numbers do not establish matched-workload gains.
Before sub-field clock interpolation, the service bridge passed the original/native Time Manager contract on
fixed 68020 and the VBL contract on cached 68040, including register checks and
eleven rejection controls for each fixture. Current machine checks require the
selected preset's data-cache setting. Full cadence acceptance remains open.

## Sub-field clock validation

The current working implementation interpolates TickCount within each PAL field
using shared `VideoTicks` arithmetic. A field-only 6/5 accumulator skips tick 5
in each six-tick group; a repeated five-tick deadline can therefore wake at six
ticks. Shared host tests cover every microsecond over 100 fields and tick wrap.
This arithmetic result does not establish native performance.

A native backward-clock control exposed scanline zero before the VBI epoch
caught up. The sampler now rejects that ambiguous position and HP 0/1, and
retains a named monotonicity failure. A cached-68040 gameplay capture completed
with no clock fault, but its full integration gate did not pass: title phase 35
has no validated paired reference. The attempted phase-35 original capture
failed its initial-cue assertion and is not accepted evidence.

The latest fixed-68020 VBL fixture reached its completion marker with no clock
fault, but observed 31 ticks and 31 dormant-task decrements. The checker requires
30 and rejected it. This is an outstanding fixture/clock investigation; neither
a completion marker alone nor the earlier field-only results validate the new
clock. Current fixture checkers also reject nonzero clock faults.

The owner approved performance item 13 before finishing timing item 11 and
gameplay item 12. The existing diagnostic measurements are preliminary evidence;
release baselines on fixed 68020 and 68030, matched A/B measurements, and measured
Fast RAM requirements remain necessary for item 13.

## Measurement build

The Amiga makefile accepts explicit `PROFILE=1 VERIFY=0` with integration and
timing probe defines. This follows the other ports' separation of probes from
verification: original input seams, timing records and phase samples remain,
but runtime SHA-256, trap tracing and expensive verification counters must be
absent from the linked binary. The link audit enforces that absence. Timing
records report zero in the unavailable trap-count column. These measurements
are still instrumented; they are not unmodified release throughput.

`VERIFY=1` remains required for correctness fixtures. The profiling build does
not perform their exit CODE/heap audits and cannot pass those gates in their
place. Ordinary `make release` retains neither measurement nor verification
probes, and the interactive launcher rejects a profiling build.

A diagnostic 68030 baseline uses the shared 50 MHz, 8 MB Fast RAM preset with
data cache and cycle-exact emulation enabled. The five updates after the
first-room checkpoint took 25 PAL fields (100 ms per update), with zero Right
reads in that window. First-room pixels, AGA planes, context words and RNG match
the pinned Macintosh reference; the route completed with no clock fault and
restored hardware. Title phase 64 is not paired with a validated original
capture, so full integration remains unverified. This baseline used `VERIFY=1`
and cannot be compared as release throughput against the new profiling mode.

The first fixed-68020 `PROFILE=1 VERIFY=0` run completed the route with no
clock fault. Its five-update window took 120 PAL fields (480 ms per update),
consumed no Right input, and ended with the same complete context bytes as the
diagnostic 68030 window. Title phase 16 pixels/colors and first-room
pixels/planes/context words/RNG match the pinned originals. The frozen executable
contains neither runtime SHA-256 nor verification traces/counters. This mode
correctly fails the full integration checker because exit CODE/heap audits are
disabled; the targeted comparisons above are not a replacement for that gate.
A matching 68030 profiling-mode baseline is still needed before comparing CPU
performance. Ordinary release was rebuilt and passed its no-verification audit.
