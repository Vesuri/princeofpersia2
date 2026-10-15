# Performance

The current owner-directed target is `a1200-030`: the shared 50 MHz 68030,
AGA, PAL, 2 MB Chip RAM and 8 MB Fast RAM preset with data cache enabled.
The owner brought 68020 item 15 forward; its baseline below fails cadence.
These are emulator measurements, not
real-hardware performance claims. The Fast RAM requirement is 8 MB (D5).

## Measurement boundary

The explicit `PROFILE=1 VERIFY=0` build retains fixed-seed integration input,
frame records and phase samples while omitting runtime hashing, trap tracing and
exit CODE/heap audits. Ordinary release has neither profiling nor verification
probes. The link audit checks the absence of expensive verification. The
measurement checker does not replace the full correctness gate.

The window begins after the first playable room, with one scene already recorded,
and ends after five further completed updates. No debugger stops occur inside
that window. Require zero Right reads, matching initial reference pixels, planes,
colors, RNG and context words, positive route completion, no clock fault,
restoration, and the effective CPU/cache configuration. A/B comparisons additionally
require identical final screen/context bytes and dirty conversion counts.

Build and run explicitly:

```sh
. amiga/env.sh
make -C amiga PROFILE=1 VERIFY=0 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_TIMING_PROBE'
AMIGA_SETUP=a1200-030 CORE=1 PERFORMANCE=1 \
  PERFORMANCE_REFERENCE="$PWD/ref/mame/runs/timing-route-1" \
  DEADLINE=1200 bash amiga/run.sh
```

The reference directory must satisfy the pinned original-state checks. The
launcher freezes executable, symbols, build receipt and observer in its ignored
run directory. `tools/check_performance.py` accepts that directory, the original
reference, `--setup a1200-030`, optional `--compare` with a baseline run, and
`--controls`. Its controls reject altered completion, input, clock, restoration
and C2P evidence. Restore ordinary play with `make release` afterwards.

## Measured performance

MEASURED: the pinned parent (`baf9642`) takes 26 PAL fields for the five-update
window, or 104 ms/update including pacing. The current build takes 20 fields,
or 80 ms/update. Both perform five conversion passes, ten dirty rectangles and
179,328 converted pixels, with zero input in that window. Complete endpoint
screen and context bytes match. `manifest/performance-parent.sha256` pins the
parent executable, symbols and both benchmark and route endpoint captures.

The parent's full-route field sampler records 243 observations: 99 in original
game execution, 36 in TickCount, 32 in frame-end C2P, 22 in SystemTask and 17 in
SectRect. These are periodic phase samples, not instruction counts or exclusive
service costs. They identify dispatch/polling and conversion as relevant work.

The dispatcher resolves known cached native service veneers before searching
original CODE segments; original return-owner checks remain active. Large
standalone scene synchronization finishes before returning to the game, so the
first small animation update does not inherit that copy. This moves work into
scene preparation rather than reducing total copying; see [timing](timing.md).
These short, field-quantized emulator windows do not establish a general
percentage speedup or all-scene frame rate.

## 68020 evaluation (item 15)

MEASURED: two runs of the same current profiling executable on `a1200-fast`
(68EC020 at 14,187,580 Hz, instruction cache, no data cache, 2 MB Chip and
8 MB Fast RAM) complete the route and restoration. Both satisfy the existing
performance-window checks against original `timing-route-1`; neither passes
`check_timing_cadence.py`. All six cadence-checker rejection controls pass.

| Run | Five-update PAL fields | ms/update, including pacing | Five-tick intervals met |
|---|---:|---:|---:|
| `.run-2awJXm` | 107 | 428 | 0 of 53 |
| `.run-c11jR8` | 106 | 424 | 0 of 53 |

Each route's timer intervals range from 7 to 29 ticks against the original
five-tick contract (about 83.3 ms). Both five-update windows perform five C2P
passes, ten rectangles and 179,328 converted pixels, with zero input and no
conversion outside frame end. Window screen/context bytes agree between runs
and with the pinned parent captures. Death screen/context also agree between
these two runs; the death screen differs from the older pinned parent, although
its context matches. That full-route picture comparison remains a failure.
No new gameplay exception is inferred from D12.

The build uses `PROFILE=1 VERIFY=0`: no runtime hashing, broad trap tracing or
exit CODE/heap audit, and no debugger stops inside the measured window. These
are emulated-field measurements, not host elapsed times or real-hardware results.
`manifest/performance-68020-baseline.sha256` pins both runs relative to the
repository root, including frozen executable/symbols, observer, machine settings,
positive completion and captures. Original inputs retain their existing pins.

The repeat confirms a substantial gap, not a marginal missed deadline.
Recommendation: extend item 19's matched-workload profiling to this 68020 target
before attempting item 15 optimization. Attribute original execution, service
handling and presentation costs and establish feasible savings first. This is
not proof that 68020 support is impossible. No runtime optimization was applied;
the ordinary release build is restored. Item 15 remains open, including timing
services, matched gameplay regressions and supported hardware requirements.

## Frame-end conversion

The renderer uses Mikael Kalms's `c2p1x1_8_c5_gen` adapter on normalized dirty
rectangles, with the shared blitter synchronization path used by Alone in the
Dark. Vette also defers conversion to completed frames. One publication pass may
call the rectangle routine several times; unchanged screen areas are not
converted merely to make one rectangle call.

MEASURED: the original health-meter path can draw before DrawWorld. Its leaf
callees do not create LINK frames, so identifying the owner only from DrawWorld's
GetGWorld misses that drawing. The port now recognizes FrameAdv's live frame by
its caller return at CODE 2+616E, guarding the original call, entry and return
instructions. It retains the owner, parent and return address until that
invocation finishes. The existing DrawWorld guard also covers initial rendering.

The current tested route has 59 updates and 59 conversion passes, 113 rectangles
and 850,272 converted pixels, with no conversion outside frame end. Benchmark
and death screen/context bytes match the pinned parent. Converted bitplanes match the chunky image at the
benchmark endpoint and death checkpoint. Sixteen altered window, route and
parent-identity records are rejected by the checker.

This establishes batching for the measured gameplay route, not coverage of all
levels or cutscenes. Title pairing, broader cadence and gameplay coverage remain
in their separate open items.

## Memory and correctness

The unmodified release build reaches the original opening checkpoint and matches
its complete viewport on the 8 MB Fast RAM preset. With the same CPU/cache
settings and 4 MB Fast RAM, it positively stops at `CORE / MEMORY`, without an
exception. Effective emulator settings confirm both memory sizes. D5 therefore
requires 8 MB Fast RAM; this is the supported configuration, not a claim that
every intermediate memory size was tested.

Profiling checkpoints leave 3,165,864 bytes of Fast RAM free at the opening/title
and 3,097,600 bytes at the first room/death; the latter largest contiguous block
is 3,096,224 bytes. Chip RAM has 1,147,864 bytes free. These are checkpoint
snapshots, not continuous high-water measurements or full-game coverage.

The current 68030 Time Manager and VBL fixtures match their pinned Macintosh
contracts, including user-mode ABI, CODE/heap audits and hardware restoration.
Both reject all twelve altered-evidence controls. Host helper tests and shared
source pins pass. Full gameplay coverage and fixed-68020 cadence remain in their
separate open items.

## Opening-sequence profile (item 19)

The first profiled scene is the start of play without input: the Kid comes
through the window and lands on the roof, a guard attacks from the left and
hits him three times, and the Kid falls off the roof. The window runs from the
first room (integration stage 3) to the original `AddKidDeathSong` kind-3 return
(stage 4), 81 game frames. Phases end at game events, never frame numbers: the
Kid's first landing (y 226, A5−50CC) and each Kid health change (A5−50B4).

| Phase | Frames | Content |
|---|---:|---|
| 0 | 6 | through the window to the landing |
| 1 | 28 | standing; the guard approaches and strikes |
| 2–3 | 9 + 9 | health 2 and 1 under attack |
| 4 | 29 | health 0, fall off the roof, until the death song |

```sh
. amiga/env.sh
amiga/aprof.sh opening-030-b                  # exact-cycle 68030, one profile per phase
amiga/aprof.sh opening-020-a a1200-fast       # 68020 (item 15)
APROF_CYCLES=0 amiga/aprof.sh cadence-x       # ordinary emulator, unmodified a1200-030
python3 tools/aprof_report.py opening opening-030-c --compare opening-030-b \
  --original ref/mame/runs/opening-noright-1
python3 tools/aprof_report.py report opening-030-b --phase 0
python3 tools/aprof_report.py work opening-030-b --phase 0     # outside the timer wait
```

`aprof.sh` builds `PROFILE=1 VERIFY=0` with the no-input route and the
`POP2_OPENING_PROFILE` recorder; ordinary release contains neither. The recorder
buffers one row per completed frame in Fast RAM (counter, ticks, fields, phase
and the 62-byte Kid record) and the observer stops only at phase boundaries.
Outputs go to ignored `tmp/aprof/NAME/`; existing names are refused. `run.sh`
accepts the run only with route completion, zero Right reads, the kind-3 death,
consecutive phase boundaries, hardware restoration and no clock fault.
`aprof_report.py` is adapted from Alone in the Dark's report (same profile
format and call-tree rules) with this port's segment, MacsBug and cached-service
names. Flat PC times are exact; call-tree attribution, including `work`, is
approximate because of tail jumps and interrupts.

The original Macintosh run `opening-noright-1` (`POP2_AUDIO=route
POP2_TIMING_TRACE=1 POP2_DISPLAY_ROUTE=1 POP2_NO_RIGHT=1`) reaches the same
death and requests timer 1 with **34 five-tick delays followed by 47 six-tick
delays**: the original deliberately slows to six ticks from the first hit. It
meets every request. Native runs request the identical delays (the native
window starts one frame earlier). Cadence is therefore judged per requested
SetTimer interval, not against a fixed five ticks.

FS-UAE deterministic mode fails GLib queue assertions and never serves the
debugger stub with either emulator, so repeats establish reproducibility. The
cycle profiler refuses an MMU model, so `a1200-030` is profiled with
`PROFILE_NO_MMU` (the option must follow the preset arguments). MEASURED: the
ordinary emulator with the same no-MMU model reproduces the profiler run (393
fields, the same late landing frames), while the unmodified preset is faster in
the landing (24 against 35–36 fields). The profiles are thus a conservative model
of the selected preset; neither establishes hardware timing.

### Baseline before reductions

MEASURED, all runs with identical Kid records in all 81 frames:

| Run | Machine | Fields | Late requested intervals |
|---|---|---:|---:|
| `opening-030-cadence-a/b/c` | `a1200-030`, ordinary emulator | 375–376 | 0 (b, c); a predates the request dump |
| `opening-030-b`, `-c` | 68030 no MMU, cycle profiler | 393–394 | 10 of 81 (landing, first hit) |
| `opening-030-nommu-control` | 68030 no MMU, ordinary emulator | 393 | landing as above |
| `opening-020-a`, `-b`, `-cadence-a` | `a1200-fast` | 964–967 | 81 of 81 |

The selected `a1200-030` preset meets the original cadence in this scene. CPU
time per frame from the exact profiles (`report`, all work; `work` excludes the
original wait loop CODE 17+31B8, whose polling is idle time):

| Phase | 68030 total | 68030 outside wait | 68030 original | 68020 total | 68020 original |
|---|---:|---:|---:|---:|---:|
| 0 landing | 143.1 ms | 143.1 ms | 23.0 ms | 499.5 ms | 99.8 ms |
| 1 stand-off | 86.2 ms | 66.9 ms | 12.0 ms | 258.8 ms | 49.3 ms |
| 2–3 attack | 97.9–99.1 ms | 65.3–66.5 ms | 12.2–12.5 ms | 252.4–255.5 ms | 48.6–49.1 ms |
| 4 fall | 97.5 ms | 40.3 ms | 8.5 ms | 153.9 ms | 28.5 ms |

The budgets are 83.3 ms (five ticks) and 100 ms (six ticks). Port work is four
to five times the original game's instructions. DERIVED: on the 68020 the
original's own landing work (99.8 ms/frame) already exceeds its five-tick
budget, so no port optimization alone can meet the landing cadence there.

68030 landing (phase 0) costs per frame. Inclusive rows overlap: the veneer
and publication costs contain dispatch, waits and helpers listed separately.

| Cost | ms/frame | Evidence |
|---|---:|---|
| `pop2_cached_services` veneers (`SectRect` 217, `UnionRect` 36, `ClipRect` 33, `EmptyRect` 26 calls) | 38.4 | 123 µs per call through Line-A `$AFFE`, full dispatch and an argument `memmove` |
| Frame publication | 35.0 | Kalms C2P 18.6; `PrinceScreen::synchronize` blitter wait 14.1 |
| Trap entry and dispatch bucket (exclusive) | 30.6 | `pop2Dispatch` ~220 and `pop2UserDispatch` ~120 instructions per trap |
| `pop2WallTicks` + `selectVBL` on every trap | 8.6 | five Chip-bus beam/CIA reads per sample, 30–54 cycles each |
| Audio driver calls | 9.2 | 14 `NativeAudio::dispatch` at 654 µs with Paula DMA waits; effect conversion 3.0 |
| `NewHandle` | 4.1 | 3.5 ms per call (15.9 ms on 68020); heap coalescing in resize/dispose |
| Original game code | 23.0 | `SaveRect`, RLE blitters CODE 20+03C0/+083A |

Trivial traps (`SystemTask`, `HLock`, `HGetState`, `StripAddress`,
`SwapMMUMode`) cost 77–101 µs each on the 68030 and 340–500 µs on the 68020.
Averaged over the scene, the trap-entry/dispatch bucket is 35.3 ms per 68030
frame and 66.8 ms per 68020 frame. Item 19's plan in `docs/open-work.md` orders
the reductions. The pins are in `manifest/opening-profile.sha256`.

### Direct pure rectangle entries (19.1)

`GetTrapAddress` returns direct Pascal entries for `SectRect`, `UnionRect` and
`EmptyRect`. They preserve the original Boolean padding, stack cleanup, measured
D0/D1 results and CCR, with stack, input/output range and return-owner checks.
A failed check restores the entry registers and enters the existing `$AFFE`
veneer for a named stop. `ClipRect`, ordinary Line-A calls and the existing
LoadSeg/Exit patch dispatch remain on the full path. No original game code or
frame publication policy changes.

Alone in the Dark's quick-trap ABI and patched-entry selection were inspected
first, along with Vette's runtime. Geometry uses AmigaXDev's `RectBounds` with
its explicit load/store accessors and shared `BigEndianMemory` word operations.
The canonical accessor extension is pinned in `manifest/references.tsv` and
published on AmigaXDev's `codex/pure-rectangle-accessors` branch. It was prepared
and installed to an isolated prefix from a clean checkout because the main
working tree has unrelated changes. Default byte-access overloads remain for
existing consumers; no independent geometry implementation was added here.

MEASURED on the same exact-cycle, no-MMU `a1200-030` model described above:
`rect-before-a/b` and `rect-final-a/b`, two completed runs on each side. Mean
inclusive time per cached call over all 81 frames:

| Service | Calls per run | Before (µs) | After (µs) |
|---|---:|---:|---:|
| SectRect | 8,725 | 126.39–128.71 | 13.88–13.91 |
| UnionRect | 1,297 | 122.60–130.28 | 13.84–13.92 |
| EmptyRect | 1,096 | 98.60–106.68 | 7.85–7.96 |

All three pass the ≤15 µs mean-call gate in both final runs, including their
ownership checks. The following ranges cover the two repeats, in ms/frame.
`work` excludes the original wait loop, so reduced work can become additional
idle time while the original requested cadence stays intact.

| Phase | Before total | After total | Before outside wait | After outside wait |
|---|---:|---:|---:|---:|
| 0 landing | 137.9–142.9 | 112.1 | 137.9–142.9 | 112.1 |
| 1 stand-off | 85.9–86.3 | 83.2–83.5 | 67.0–67.2 | 51.1–51.5 |
| 2 attack | 97.4–98.7 | 98.3–98.5 | 67.9–68.5 | 51.4–52.5 |
| 3 attack | 98.7–99.3 | 98.6–98.8 | 67.8–68.3 | 50.6–51.6 |
| 4 fall | 97.4–97.5 | 98.2 | 40.3–41.1 | 29.9–30.4 |

The window takes 392–393 fields before and 381 after. All four runs have the
same 81 Kid records, counters/phases, first-room pixels/palette and death pixels;
both pairs pass the repeat check. Original timer requests match
`ref/mame/runs/opening-noright-1`. Late intervals in this conservative model fall
from 9–10 to 6–7 of 81; this step does not close item 19's final cadence gate or
claim real-hardware timings. The profiler now resolves a function that aliases
the cached-table end marker and gives direct rectangle bodies their own bucket.

The authored `mac_rectangles.lua` / `POP2_RECTANGLE_PROBE` fixture pairs 1,220
calls with System 7.5.5 (`rectangles-original-final`): signed extremes, negative
bounds, empty/inverted/touching/overlapping rectangles, both input aliases and
a separate destination, and two incoming CCR patterns. It compares rectangle
bytes, all data registers, Boolean padding and CCR; the native fixture also
checks A2–A6, exact Pascal stack cleanup and 13 rejected stack/owner/pointer
conditions. The fixture calls the production entries on the real game stack,
resumes the route and requires hardware restoration. It is excluded from release.
The final native fixture passes all of these comparisons and resumes to the
normal route-completion stop. The original normalized contract is pinned in
`manifest/pure-rectangles-reference.sha256`; native fixture and all four profile
inputs/results are pinned in `manifest/pure-rectangles.sha256`.

The fresh integration attempt passed all four service contracts but rejected
its first gameplay reference at the existing A5 pin: `20261015-151738-f9137e07`
differs only at A5−0B77 outside the already normalized clock words. Its pixels,
palette and context match. The failure is retained under item 16; no pin or
normalization was weakened. The integration regression uses the documented
pinned-reference mode and explicit `--setup a1200-030`, propagated through
all native checks and rejection controls. The old harness assumed every native
logo capture would occur at frame 16, despite using a controlled frame-16 original
renderer fixture. The title comparison now uses the same guarded frame-15 cue
input in a separate verification-only native run. Natural gameplay repeats and
the no-Right control exclude that define and retain their full first-room,
input, completion and restoration checks.

The common regression gates pass on `a1200-030`: the complete integration gate
(state-paired title plus two natural runs and no-Right control), VBL and Time
Manager fixtures with 12 rejected alterations each, and 58 consecutive original
five-tick cadence intervals with six rejected alterations. Integration rejects
20 altered native title fixtures, 19 altered natural-route fixtures and six
altered original title fixtures. EqualRect's 36 original cases, portable helper
checks and shared sanitizer tests also pass. The ordinary release link/symbol
audit is unchanged and passes; `make release` restores ordinary play. The final
profile executable remains byte-identical after the verification-only title
harness changes. No new fixture or title-state probe is present in release.

Reproduce the paired fixture (choose an unused original run name):

```sh
. amiga/env.sh
POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1 POP2_RECTANGLE_PROBE=1 \
  python3 tools/run_reference.py --name rectangles-original-repeat
make -C amiga VERIFY=1 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_RECTANGLE_PROBE'
AMIGA_SETUP=a1200-030 CORE=1 \
  RECTANGLE_REFERENCE="$PWD/ref/mame/runs/rectangles-original-repeat" \
  GDBSCRIPT="$PWD/amiga/rectangles.gdb" DEADLINE=1200 bash amiga/run.sh
```

For each of the four profile names above, run `aprof_report.py opening NAME
--original ref/mame/runs/opening-noright-1`, then `report NAME --phase N` and
`work NAME --phase N` for N = 0 through 4. Use `opening rect-final-b --compare
rect-final-a --original ref/mame/runs/opening-noright-1` for the final repeat and
`calls NAME pop2SectRect` (or `pop2UnionRect`, `pop2EmptyRect`) for inclusive
costs. Before-change call labels are `CACHED SectRect`, `CACHED UnionRect` and
`CACHED EmptyRect`. Restore ordinary play with `make release` after probes.

### Audio call cost (19.6)

The candidate reuses immutable instrument conversions through the shared bounded
cache, uses the existing shared span converter for mutable one-shot effects,
and records settled/stopped Paula state. Immediate replacements retain the
two-sample-period DMA-off wait but omit the intermediate DAC-zero handshake;
completed voices and hardware handoff still fully quiesce the DAC. Vette's
immediate restart and Alone in the Dark's retained samples/quiet-channel state
were inspected first. The audio and common gates pass on the combined working
candidate; ordinary dispatch item 19.2 remains open.

The complete audio gate passes in `tmp/audio-gate-8k7g_r3q`: original allocation
contracts, 7,549 cutscene calls, two identical 9,329-call route replays, 62
release tails per replay, ten evidence controls, six runtime controls, the
original byte-guard rejection and the 144-call startup regression. The release
audit passes after that suite. Callback-order fixture `.run-G8KcYK` matches the
original seven-call control and passes all eighteen altered-evidence checks.
The common gates were checked separately from these audio replays.
The current integration gate passes in `tmp/integration-gate-6d9rdclj` on
`a1200-030`: paired title rendering, two identical gameplay frame captures,
the no-Right outcome control and 45 altered-evidence controls. The host suite
and shared helper hashes pass on this candidate.

MEASURED phase-0 inclusive `NativeAudio::dispatch` cost, using the existing
audio-call metric rather than the broader exclusive Audio bucket:

| Captures | Calls in six landing frames | ms/landing frame |
|---|---:|---:|
| `dispatch-owner-fixed-a`, `audio-before-b` | 63, 64 | 7.89, 8.08 |
| `audio-restart-a`, `audio-restart-b` | 62, 59 | 1.989, 1.814 |

The before pair uses the same frozen dispatch/clock candidate. The after pair
passes its repeat comparison with 81 identical player records, original timer
requests and phase cadence within tolerance. Call counts differ between the
captures; these are scene costs, not equal-call microbenchmarks.
An intermediate span-conversion pair measured 2.219/2.208 ms and failed the
budget. Its comparison with the before capture also exceeded phase-0 repeat
tolerance; retain that failed report. Per-phase `opening`, `report` and `work`
outputs remain ignored. VBL (`.run-B4P3Uj`) and Time Manager (`.run-KuR8nM`)
fixtures match their originals and each reject twelve alterations. Cadence
(`.run-3jnfw4`) matches 59 consecutive original five-tick intervals and rejects
six alterations. The restored release passes link and diagnostic-exclusion audits.

## Scenes to retain for future profiling

Record suspected heavy scenes encountered during gameplay validation here,
including the level/room or cinematic, reproducible route or save, observed
symptom, target/build and whether a matched-workload measurement exists.
Debugger waits and verification-run wall time alone do not establish a slowdown.
No additional heavy scene has yet been established by the current route evidence.

The level-five carpet exit is a useful transition workload, not a measured
hotspot: the paired route in `docs/gameplay-coverage.md` covers NIS 1 returning
to level six. `Profile 05` supplies a level-entry starting point, not a save
immediately before the carpet. Retain it as a comparison workload when measuring
any suspected transition slowdown.

The level-eight throne-room dissolve is another candidate workload. The original
generates/persists dissolve data when its cache is absent; retain both a clean
cache and an already generated cache for comparison. The pinned clean route in
`manifest/gameplay-assisted-level8-clean.sha256` and its visible cutscene captures
in `manifest/gameplay-throne-reference.sha256` reproduce the scene. Measure
release execution on `a1200-030` before calling it a bottleneck; current debugger
and verification wall times are not performance measurements.
