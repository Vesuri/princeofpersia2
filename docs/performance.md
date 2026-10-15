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
