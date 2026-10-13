# Timing

Item 11 remains open: full-game step cadence has not matched the Macintosh
reference on the required 68030 and faster CPU configurations. The service fixture below verifies
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

The current fixed-68020 VBL fixture passes with 30 elapsed ticks, consecutive
callbacks, the original dormant countdown/removal results, user-mode ABI, unchanged
CODE and restored heap/hardware. All twelve rejection controls pass. This is the
current build result; prior overshoot observations do not establish a current
service failure. The 68030 Time Manager and VBL fixtures also pass their pinned
contracts and twelve rejection controls each.

## Current cadence gate

Two completed original routes reproduce 100 timer-1 requests of five ticks,
with all 99 intervening intervals exactly five ticks. The normalized contract is
pinned in `manifest/timing-cadence-reference.sha256`; absolute epochs and emulator
addresses are excluded. `tools/check_timing_cadence.py` checks the profiling
route's positive completion, initial reference state, converted bitplanes,
consecutive original frame counters and every recorded timer interval, including
the first. Six altered-record controls cover missing steps, changed timing and
changed timer requests. A synthetic positive is only a checker control.

MEASURED current native results, using the profiling build from `fb9fcf3`:

| Configuration | Five-update window | Timer interval results | Cadence gate |
|---|---|---|---|
| Fixed 68020 (`a1200-fast`) | 113 PAL fields / 452 ms per update | 53 intervals of 7–31 ticks | Fails |
| 50 MHz 68030 (`a1200-030`) | 24 fields / 96 ms per update | First interval 11 ticks; next 56 intervals five ticks | Fails first interval |
| Cached 68040 (`a1200-040`) | 32 fields / 128 ms per update | 45 intervals five ticks; nine intervals 6–11 ticks | Fails |

These are instrumented emulator results. The 68040 emulator's timing model is
not a hardware speed prediction. All three five-update endpoints have identical
context bytes, and each route converts only at frame end. The slower 68020 route's
death image differs from the 68030 parent, so it does not pass the complete
parent-route image comparison. Performance-window completion does not imply
cadence acceptance. Neither the initial overrun nor later overruns are excluded
from the timing gate.

Run the gate after the explicit profiling route described in
[performance measurements](performance.md):

```sh
python3 tools/check_timing_cadence.py NATIVE_RUN ORIGINAL_REFERENCE \
  --setup a1200-030 --controls
```

Use the actual selected CPU preset. The current runs correctly fail with
`TIMING CADENCE / REFERENCE CADENCE INCLUDING FIRST STEP`. Item 11 remains open;
The owner deferred 68020 support to final item 15. Item 11 now targets
`a1200-030` and faster configurations; D5 records that current target.

The service-return path skips beam sampling when the VBL queue is empty; first
installation and final removal still establish/reset the queue epoch. It also
uses the existing audio pending-work predicate before polling audio. The final
fixed-68020 A/B before deferral measured 113 versus 112 fields for the same five
updates, with identical benchmark and death screen/context bytes. That one-field
difference is not a general speed claim. The complete VBL fixture and all twelve
rejection controls pass with the change. Further 68020 work is deferred to item 15.

## Measurement build

The explicit `PROFILE=1 VERIFY=0` build retains integration input, timing records
and phase samples, while omitting runtime hashing, trap tracing and expensive
verification counters. The link audit enforces this boundary. Ordinary release
has neither measurement nor verification probes. `VERIFY=1` remains required for
the correctness fixtures; profiling does not replace their exit CODE/heap audits.

See [performance measurements](performance.md) for the 68030 matched workload,
parent comparison, dirty-rectangle conversion checks and measured RAM requirement.
Those bounded measurements do not establish title phase pairing, all-scene
cadence, full-game coverage or real-hardware performance.
