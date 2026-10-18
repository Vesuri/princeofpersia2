# Performance

The current owner-directed target is `a1200-030`: the shared 50 MHz 68030,
AGA, PAL, 2 MB Chip RAM and 8 MB Fast RAM preset with data cache enabled.
68020 item 15 is required before 0.90 (D14); its earlier baseline below fails cadence.
The retained accelerated measurements below use the legacy explicit-frequency
calculation; item 22 requires requalification with the corrected clock.
These are emulator measurements, not real-hardware performance claims. The Fast RAM requirement is 8 MB (D5).

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

### Shared changed-row screen copies

MEASURED: the current screen CopyBits path uses AmigaXDev's optional change
report and bounded 68020 `CopyChangedRows.inc` body. It compares disjoint row
ends, copies the differing middle and reports conservative four-byte bounds.
Sixteen-row bands feed the existing dirty-rectangle planner. No comparison
framebuffer is allocated. Masked, mapped, scaled and overlapping forms retain
their existing paths. Viewport changes force a full viewport update; palette
changes still publish even when pixel rows are unchanged. Diagnostics remain
opt-in, and ordinary release startup has no added verification work.

The final `item15-shared-row-copy-020-a/b` pair (`.run-qtT8b3` and
`.run-wOgdGB`) measures **253.07/253.30 ms per landing frame**, versus
262.52–263.27 ms before this change. Both take 537 PAL fields and retain all
82 baseline player records and endpoint images. The same-build repeat passes;
57/82 requested intervals remain late and the strict original comparison still
rejects the extra death-phase request. Item 15's 83.3 ms landing gate is not met.
Flat A costs are 34.58 ms conversion, 14.77 ms CopyBits and 13.00 ms other
presentation per frame. Inclusive costs remain unverified.

The shared sanitizer suite passes, including 8,192 clipped copy/report cases
and insufficient-capacity, missing-callback and overlap fallbacks. The actual
linked native helper in `tmp/changed-rows-native-hrurys0i` passes 1,666 cases:
widths 0–640, every source/destination byte alignment, unchanged/interior/edge/
full changes, zero height, the full 640×480 extent, complete destination and
padding checks, source preservation, conservative spans and callee registers.
The reusable API/body are pinned to AmigaXDev commit `ef10397` and installed
through its shared workflow. Neither donor game tree was modified.

The final frame-boundary capture `.run-oNlGlt` compares with the unchanged
baseline `.run-i7zWFM` at all **82 game-frame boundaries**, including 21 with
no pending publication. Every full viewport, decoded planar image, copper
palette, colour table and 64-byte player record matches. Eight corrupted-evidence
controls pass. First-room pixels, planes, palette, copper, RNG and curated
context also match the pinned Macintosh reference
`ref/mame/runs/integration-natural-title-1`. This profile-mode capture verifies
restoration without a heap audit; it does not claim a new title or timing gate.
The earlier strict 82-publication failure is retained: publication count is no
longer the game-frame count, and the new check compares every game frame instead.

Reproduce the native fixture with
`python3 tools/run_wrapper_fixture.py tools --kind changed-rows`. Use
`tools/capture_c2p_bounds.py --frame-boundaries --setup a1200-fast` for stopped
correctness captures, then `tools/check_frame_boundaries.py RUN --compare BASE`
and `tools/test_frame_boundaries.py RUN`. These debugger-stopped captures are
never used for timing. Host tests and `make release` pass.
`manifest/performance-68020-shared-row-copy.sha256` pins the evidence and frozen
build; the earlier pilot remains pinned separately.

### Further 68020 candidates

MEASURED: isolating `callbackService` with `noinline` produces a 254.08 ms
landing pilot (`item15-dispatch-split-020-a`, `.run-WM6Yj3`), with 52.35 ms in
flat dispatch cost. It preserves all 82 baseline player records and endpoint
images but establishes no useful saving; the change is removed. The flat
profile also shows that `pop2QuickSwapMMUModeWarm` itself costs only 0.58 ms per
frame. Unverified inclusive trap totals must not be treated as the cost of that
small body or as a promised saving from replacing a rendering wrapper.

MEASURED: a separate framebuffer-bias pilot aligns the measured x=65 viewport
by adding three bytes to a privately padded screen allocation. Its one run
(`item15-screen-alignment-020-a`) measures 251.49 ms landing, including 33.29 ms
conversion, versus 34.58 ms conversion in the retained build. All 82 player
records and endpoint images match. This small single-run result is unqualified;
it has no full frame-boundary or repeated timing gate and is removed. Production
allocation and addresses remain unchanged.

MEASURED: the retained build's flat PCs charge 1.94 ms per landing frame to
`BankRLELR`'s pointer decrement/DBRA transparent-run loop at CODE 17+$010E.
An ignored eight-byte candidate replaces that loop with a count-based pointer
adjustment. The whole original inner block and candidate pass 448 paired native
cases (14 captured calls × 32 initial CCR values) in
`tmp/bank-skip-native-k73rqdr3`: complete framebuffer, 56 frame/argument bytes,
15 registers, CCR and stack output agree. The candidate changes scratch D5 high
bits and transient flags inside the loop; the original enclosing block restores
D5 and overwrites the flags before its exit. This is evidence at the full-block
boundary, not a claim that intermediate register states are identical. Additional
run-length/clipping boundary fixtures, an actual Macintosh pairing, guarded
installation and matched game timing remain required. No game patch is installed
and no candidate speedup is claimed. Curated names are in `disasm/symbols.csv`;
`manifest/performance-68020-dispatch-render-candidates.sha256` pins the captures,
prototype and input identities. The verified ordinary release is restored.

### Bounded sprite-cache feasibility

MEASURED: the final Kalms executable, observed in `.run-BW5Tb2`, completes
the opening route and restoration with 82 recorded frames. Breakpoints capture
only landing calls to `PaintRLE` and `BankRLELR`: 78 calls, 4,625 requested rows,
247,453 requested pixels and 107,605 encoded bytes. This stopped execution is
an input census, not a timing measurement. Curated routine names are recorded
in `disasm/symbols.csv`.

DERIVED: retaining every previously seen encoded row by object and row address,
and requiring its bytes to match, finds 2,178 reused rows representing 79,088
requested pixels (32%). There are 2,447 distinct rows. This is an unbounded
input-reuse estimate, not a measured cache speedup: comparison, decoding on
misses, storage, eviction and drawing still cost work. An exact-window cache
finds 28 repeated calls representing 65,699 pixels. No sprite cache is installed;
this evidence does not justify implementing a large cache as the next step.
The capture, observer, analysis and executable identities are pinned in
`manifest/performance-68020-rle-census.sha256`.

### Rejected pixel-address quick-entry pilot

MEASURED: `item15-pixel-query-020-a` completes the calibrated opening route in
573 PAL fields, with 60/82 late intervals. Landing costs 272.86 ms per stored
frame, versus the retained final Kalms pair's 275.91–275.99 ms. The single pilot
does not establish a repeatable gain. All 82 Kid records and first-room screen,
palette and death screen match the retained final Kalms C baseline. The strict
original comparison still rejects the requested delays.

The pilot reused Alone in the Dark's quick Toolbox return convention (consumed
Pascal argument bytes plus one; zero means fallback), its guarded C-bridge
approach and this port's existing GetPixBaseAddr service body. Only selector
`$0004000F` inside an owned display frame with no due services was eligible.
The roughly 3 ms difference is too small to justify qualifying this isolated
shortcut against the remaining gap. The pilot is removed and release restored;
no new fast-entry ABI or complete display-correctness claim is made. Source
patch, frozen executable, capture, reports and endpoint comparison are retained
under `tmp/` and pinned in `manifest/performance-68020-pixel-query.sha256`.

### Rejected forward-RLE C pilot (item 15)

The original PaintRLE block (CODE 17+$5842..$5917) is already assembly.
An isolated authored C++ decoder using shared BigEndianMemory, earlier longword
fill/copy thresholds and one clipping calculation per encoded run passes
2,048 original-instruction/native cases in `tmp/rle-pilot-native-vl0gdyd5`.
The 64 pinned forward-RLE calls come from the landing census; each exercises
all 32 incoming CCR values. Full framebuffer bytes, 48 bytes of frame locals
and arguments, all 15 registers, return CCR and SP agree. The contract is
restricted to these positive dimensions and nonoverlapping input/output.
This is execution of original instructions on the native test machine, not a
new Macintosh reference fixture or a general replacement qualification.

MEASURED: standalone calibrated-68020 A/B/B/A execution of the same 64-call
batch, repeated 32 times per sample, takes **89.637/89.645 ms** with the original
and **158.953/158.957 ms** with the candidate. ReadEClock reports 709,379 Hz
throughout. The replacement costs **1.773 times** the original. This includes
the identical invocation/setup harness, excludes framebuffer initialization,
and does not measure in-game cadence. There is no justification for game
integration: the C++ pilot is rejected and the production renderer is unchanged.
Vette's guarded row-loop replacement and both ports' existing memory helpers
were inspected; the pilot reuses the shared memory operations and native wrapper
fixture. No shared implementation or other port was changed.

Two earlier setup attempts are retained as failures: `f5n9a19s` supplied invalid
D0 upper bits, and `fg7ed_mc` placed the synthetic stack across the C callee's
input frame. Both were stopped through their recorded shared-helper PID.
The final fixture supplies the original post-SwapMMUMode D0 state and a stack
below the input frame, outside the framebuffer. Only its explicit 2,048-case
completion is a pass. Source, inputs, failed setups and final timing are ignored
and pinned by `manifest/performance-68020-rle-pilot.sha256`.

### Rejected batched CPU/blitter pilot (item 15)

A second layout adapts the pinned Kalms three-stage shuffle into eight word
streams per row, then submits eight constant-mask blits per 32-row strip.
Two 16 KiB Chip buffers overlap CPU preparation with the preceding strip's DMA.
The standalone `c2p-native-43duizw1` fixture passes all 193 complete pixel/ABI
cases. The in-game measurement uses the existing queue/completion machinery
through a temporary assembler-ABI adapter, which is removed with the pilot;
no shared framework interface or other port was changed.

MEASURED: `item15-hybrid-batch-020-b` costs **276.71 ms** per landing frame,
versus the retained **262.52–263.27 ms** pair, and takes 568 PAL fields with
60/82 late intervals. The CPU stage alone costs 49.09 ms, almost the complete
50.09 ms CPU-only converter cost, before queue processing and waits. All 82
player records and endpoint screens/palette match the baseline, but no complete
in-game planar qualification is claimed. The pilot is rejected and removed;
ordinary release/link audits pass. The failed assembly-macro and first game
build attempts are retained, with no emulator completion attributed to them.
`manifest/performance-68020-hybrid-batch.sha256` pins sources and evidence.

A build-setting control checked Vette and Alone in the Dark's explicit
`-fomit-frame-pointer`. This compiler already enables it at `-O2`; an explicit
rebuild produces the exact same executable. Its 262.56 ms landing measurement
is a same-binary control, not an optimization. No build flag was changed.

### Guarded drawing-service entries (item 15)

Nine drawing services now use Alone in the Dark's guarded C-bridge/Pascal
return convention: OffsetRect, InsetRect, SetRect, SectRect, UnionRect,
EmptyRect, RGBForeColor, RGBBackColor and GetPixBaseAddr. They reuse the
existing service bodies and shared rectangle/GWorld helpers. Each requires
an owned unfinished display frame and no due services. ClipRect and CopyBits
stay on the full path; the latter's long body can cross a service deadline.
A failed body preserves its error and caller identity for a user-mode loud stop,
without retrying a potentially mutated operation. Unknown-owner failures stop.

MEASURED: frozen-build `item15-quick-display-020-a/b` take **263.27/262.52 ms**
per stored landing frame, against the final Kalms pair's **275.99/275.91 ms**.
The repeat passes with 552/551 PAL fields, identical executable identities,
all 82 Kid records and endpoint screens/palette. Both match the retained
baseline's player records and endpoints. Exclusive dispatch costs fall from
64.26 ms in final Kalms C to 53.11/52.73 ms. These are flat-PC costs;
unmatched returns preclude verified inclusive call-tree claims.
Both runs still have **59/82 late requested intervals**, and the strict original
comparison rejects the extra death-phase request/frame. The 83.3 ms landing
request remains unmet; item 15 stays open.

Native exception-return fixture `tmp/pascal-native-rensyfh1` passes 192 cases
covering all 15 saved registers, all 32 CCR values, six Pascal cleanup sizes
and unchanged stack guards. It executes the actual cleanup instructions
extracted from MacEntry.s. Reproduce with
`python3 tools/run_wrapper_fixture.py tools --kind pascal` after sourcing the
shared toolchain environment. This checks the real RTE sequence and USP update
using a supervisor-mode capture, separately from live user-mode gameplay.

The final diagnostic run `.run-sb35Lg` passes 1,152 fast/full drawing comparisons,
including colour-port state, 126 guard rejections and four terminal-error
controls. Existing quick-entry fixtures, live guard audits and the restored
route pass; 22 altered-log controls are rejected. Its first-room pixels, planes,
palette, RNG and curated context match the pinned Macintosh reference.
The separate `.run-SguQrY` capture verifies every one of 82 complete planar
images against its chunky source, legal/minimal conversion coverage, and all
82 viewports byte-for-byte against the retained `.run-xk0vlH` baseline.
Host tests and the restored ordinary release/link audits pass.

Earlier `.run-TX7UTg` and `.run-w95Let` each report one synthetic comparison
failure; the latter identifies an EmptyRect rejection (result zero), not a wrong
service result. Positive fixtures now permit bounded retries only across an
observed clock-field change with unchanged guest state, and still require every
case to complete through the fast entry. `.run-BALpUH` passes with two such
fallbacks; final `.run-sb35Lg` needs none. Failure evidence remains retained.

The earlier ten-service scratch pilot `item15-display-family-020-a/b` included
CopyBits and measured 259.89/259.86 ms, but was not qualified. Its lower costs
are not the production result. Its patch and evidence remain ignored and pinned
in `manifest/performance-68020-display-family.sha256`.
The accepted nine-service evidence, failed controls and per-phase reports are
pinned in `manifest/performance-68020-quick-display.sha256`.

The pinned upstream Kalms tree also supplies `normal/c2p1x1_8_c3b1_030.s`, a
CPU-plus-blitter converter. Source inspection establishes that it calls graphics
library QBlit and assumes a different buffer layout. It is not a drop-in kernel
for this port's takeover, row-interleaved planes and shared completion queue.
Any experiment must adapt those interfaces, retain dirty conversion at frame end,
and include intermediate Chip-memory traffic and completion in its measurement.
The column experiments below test this alternative both synchronously and with
CPU/DMA overlap; neither replaces the production kernel.

### Rejected synchronous hybrid conversion

MEASURED: `item15-hybrid-c2p-020-b` completes the calibrated opening route in
605 PAL fields, with 60/82 requested intervals late. Landing costs 296.00 ms
per stored frame, against the retained final Kalms pair's 275.99/275.91 ms.
All 82 Kid records and first-room/death chunky images and palette match final
Kalms C. The strict original cadence comparison still fails. This single pilot
regresses, so it is removed; production source and release are restored.

The prototype adapts the pinned Kalms c3b1 CPU stage to 16-pixel columns at
source stride 640. A 6 KiB Chip scratch holds one column; eight synchronous
blits finish it into the existing row-interleaved planes. The host scalar model
passes 10,000 deterministic input blocks. The isolated calibrated native fixture
`c2p-native-v_1xx49d` passes all 193 existing C2P cases, checking complete output
and surrounding guards, source preservation, source alignments, edge/full-screen
extents and all C ABI saved registers. This does not establish complete in-game
planar correctness. The earlier failed native fixture and pre-launch build are
retained separately, not counted as passes.

Exclusive landing costs are 29.89 ms in the adapted CPU stage and 31.16 ms in
the aggregate shared blitter wait routine, before counting adapter setup and
other presentation work. The standard reporter omits untyped `hybridColumn`
and incorrectly labels its range with the preceding unused Kalms symbol;
the retained pilot-only reporter adds the actual ELF symbol and supplies the
corrected flat report. No inclusive call-tree costs are used for this decision.
CPU work and DMA are serialized in this prototype; the CPU-only figure is not
a speedup. The following separate experiment measures overlap.
Sources, successful/failed fixture evidence, frozen pilot and reports are pinned
in `manifest/performance-68020-hybrid-c2p.sha256`.

### Rejected overlapping hybrid conversion

MEASURED: `item15-hybrid-overlap-020-a` completes the calibrated opening in
612 PAL fields with 60/82 late requested intervals. Landing costs 289.67 ms
per stored frame, still above the retained final Kalms pair's 275.99/275.91 ms.
All 82 Kid records and first-room/death chunky images and palette match final
Kalms C; strict original cadence still fails. This single pilot is removed and
production release restored. It does not qualify complete in-game planar output.

The pilot uses two 6 KiB Chip buffers, preparing the next column while the
shared `blitterCombineWithMask` queue finishes the previous one. It drains
before scratch reuse and publication, and uses the existing completion handler.
The source/mask arrangement matches the helper used by both reference ports;
`c2p-native-ub8slkom` passes the 193-case native pixel/ABI oracle for the revised
three-channel blit mapping. That isolated synchronous oracle does not validate
the new overlap scheduling.

Exclusive landing costs are 34.40 ms in `pop2HybridColumn`, 9.91 ms in shared
masked-blit submission and 6.58 ms in the shared queue processor, before the
remaining presentation/interrupt work. Compared with the synchronous prototype,
the same 155,991 CPU-stage instructions now take longer (29.89 to 34.40 ms),
consistent with overlapping Chip-bus contention. These flat-PC measurements
do not use the unverified call tree. This implementation therefore supplies no
reason to replace the production CPU converter or qualify the hybrid further.
Evidence is pinned in `manifest/performance-68020-hybrid-overlap.sha256`.

Item 15 remains open. The fixed target is `a1200-fast`: 68EC020 at
14,187,580 Hz, instruction cache enabled, no data cache, 2 MB Chip and 8 MB
Fast RAM. The current candidate does not meet the original cadence.

### Clock calibration

MEASURED: the legacy pinned profiler reports `cycleunit 115`, rather than 128,
for this preset. AmigaXDev’s shared `fsuae-cpu-frequency.patch` corrects the
explicit-frequency calculation. The already installed register-observer build
includes that correction and reports 128, with `CYCLE_UNIT 512`, colour clock
3,546,895 Hz and CPU model 68020. The ratio corresponds to the target’s four CPU
cycles per colour clock. The legacy unit gives an approximately 11.3% faster
CPU cycle budget; its field timings are not calibrated 14 MHz evidence.

`CALIBRATED_CPU=1` selects the separately pinned shared corrected emulator for
this project’s diagnostic launcher. `FSUAE_CALIBRATED` is overridable through
`amiga/env.sh`; the selected binary must match `manifest/scaffold-inputs.tsv`.
Build that shared installation from AmigaXDev with
`scripts/build-fsuae.sh fs-uae-register-observer patches/fsuae-aprof.patch`;
its source revision and patch hashes are retained with the evidence.
The legacy default remains available for reproducing prior measurements.
This changes the host measurement tool, not game instructions or clocks.
Accelerated requalification is recorded separately in item 22.

```sh
CALIBRATED_CPU=1 amiga/aprof.sh NAME a1200-fast 1200
```

### Timing services

The corrected-clock 68020 VBL fixture `.run-9Nuaeq` passes the pinned original
countdown/removal contract, user-mode register/CCR checks, six-field deferred
callback dispatch and restoration. All 13 altered-evidence controls are rejected.
The Time Manager fixture `.run-NkFKHo` also passes the pinned original delay
units, active priming, cancellation, zero-delay, rearm, user-mode ABI and
restoration checks; its 12 altered-evidence controls are rejected. These are
service correctness results, not cadence passes. Reproduce the commands in
`docs/timing.md` with `AMIGA_SETUP=a1200-fast CALIBRATED_CPU=1`.

### Calibrated baseline before rendering replacements

MEASURED: `item15-calibrated-020-a` and `item15-calibrated-020-b` in
`tmp/aprof/` use the same post-item-19 executable and corrected emulator.
Both complete the no-input route, death-song endpoint and hardware restoration,
with no clock fault. Both record 82 frames over 660 PAL fields and miss 61 of
82 requested timer intervals. All 82 player records agree between repeats.
The six landing player records also match the earlier opening baseline.

Exclusive PC costs per recorded landing frame (phase 0):

| Cost | Repeat A | Repeat B |
|---|---:|---:|
| Entire profile window | 328.92 ms | 329.19 ms |
| Original game code | 105.04 ms | 104.96 ms |
| Trap entry and dispatch | 74.43 ms | 74.48 ms |
| C2P | 54.68 ms | 54.72 ms |
| CopyBits | 18.56 ms | 18.61 ms |
| Other presentation | 14.48 ms | 14.47 ms |

The landing request is five ticks (83.3 ms). Original-code costs include its
callbacks and rendering, not just game logic; this is not an immutable hardware
lower bound. Nevertheless, these measurements do not support closing the gate
through service optimization alone. D16 authorizes equivalent replacements for hot original rendering routines
while retaining the existing cadence gate. Vette’s byte-guarded row-copy replacement and original/register/CCR
fixtures provide the reuse precedent for the guarded replacement below.

The original comparison **fails**: the native route requests 35 five-tick and
48 six-tick delays, retaining one extra death-phase frame beyond the documented
one-frame initial observation offset. Do not normalize away that extra frame
or infer a new D12 exception. The original-request failure report is retained.
Unmatched returns also remain in the profiles; only exclusive PC costs are
used here, not inclusive or call-tree-derived active-work estimates.

The legacy-profiler controls `item15-current-020-b/c` complete in 599/597 fields,
with the same 82 player records, and report cycle unit 115. Their phase-zero
cost is approximately 291 ms/frame, but they are rejected as fixed-clock
qualification. The sandboxed first launch `item15-current-020-a` never acquired
its debugger and is an incomplete control, not a performance result.
Earlier runs in `manifest/performance-68020-baseline.sha256` and the opening
baseline below also use the legacy clock calculation. Preserve their pins;
do not compare them with corrected-clock results as an optimization speedup.

`manifest/performance-68020-current.sha256` pins the current captures, frozen
builds, machine settings, original-comparison failure, service results and
clock controls. Host tests and the calibrated-input altered-binary rejection
pass. These baseline captures precede the guarded rendering replacement below.


### Guarded SaveRect row replacement (D16)

CODE 17+$53A4..$53C5 now calls an authored native row loop, then resumes at
+$53C6. The original clipping, allocation, row stepping and header writes remain.
A load-time fingerprint checks the 34 replaced instruction bytes before writing
JSR/BRA; installation and the fixture separately verify SHA-256. No original
instruction array is compiled into the game. The ordered longword-copy macro is
shared through AmigaXDev; it preserves longword overlap semantics, including
unaligned addresses, rather than substituting byte-wise propagation.

MEASURED: `save-rect-3` runs the extracted original loop on the Macintosh.
`tmp/save-rect-native-hfcvgmeu` runs that original and the replacement on calibrated
`a1200-fast`. All 672 cases agree byte-for-byte across the full source/destination
buffer and 64 bytes of frame locals, all 15 registers, CCR and final SP. Cases
cover widths 4..32764, every address alignment, both overlap directions, exact
aliasing and all incoming CCR values. Interrupts are masked only while the
Macintosh fixture holds arbitrary test registers; the live game is unchanged.
There are 24 altered-output rejection controls. The host tests also reject all
272 single-bit alterations of the original loop, three truncated extents and
three invalid entry addresses, without partially patching rejected inputs.

The initial reference without the integration switch never reached the fixture.
The unmasked reference `save-rect-2` matched its completed output prefix but was
interrupted while holding arbitrary registers and failed completion. Neither is
accepted evidence. The shared launcher now rejects the missing-switch setup.

MEASURED: matched opening repeats `item15-save-rect-020-b/c` take 639 PAL fields
each, against 660 in both calibrated baselines. Exclusive landing-window costs
are 303.84/304.40 ms per recorded frame, against 328.92/329.19 ms. Both repeats
retain all 82 baseline player records and identical first-room and death pixels
and palette. The repeat phase-state and field-variation gate passes. They still
miss 61/62 of 82 requested intervals and retain the extra death-phase request;
**item 15 is not complete**. Only exclusive PC measurements support these claims.

Reproduce after sourcing `amiga/env.sh`:

```sh
python3 tools/build_save_rect_fixture.py
POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1 POP2_SAVE_RECT_FIXTURE=tmp/save-rect-fixture \
  python3 tools/run_reference.py --name NAME --deadline 600
python3 tools/run_wrapper_fixture.py tmp/save-rect-fixture --kind save-rect
python3 tools/check_save_rect.py ref/mame/runs/NAME tmp/save-rect-native-NAME
c++ -std=c++17 -fsanitize=address,undefined tools/check_save_rect_patch.cpp -o tmp/test-save-rect-patch
tmp/test-save-rect-patch tmp/save-rect-fixture/original.bin
CALIBRATED_CPU=1 amiga/aprof.sh NAME a1200-fast 1200
```

`manifest/save-rect-inputs.sha256` pins the generated original oracle and cases;
`manifest/save-rect-evidence.sha256` pins the successful and failed controls,
source receipts and matched measurements. Generated bytes stay ignored.


### Reusing inactive copper palettes (item 15)

`PrinceScreen` retains the fixed bitplane pointers, mode words and terminator
of each of its four copper lists. It rebuilds an inactive buffer's two field
palettes only when that buffer has not received the current RGB24 content.
The reusable `CopperPaletteCache` is shared through AmigaXDev, following Alone
in the Dark's per-list palette generations. Exact colour comparison supports
callers without a reliable seed; per-list invalidation avoids generation-wrap
aliases. Vette's inactive-list ownership was also inspected. Active lists are
never edited, palette-only publications still occur, and Kalms conversion and
completed-frame synchronization are unchanged.

MEASURED: identical-build repeats `item15-palette-020-b/c` use the calibrated
68020 and take 624/622 PAL fields, against 639/639 for the SaveRect baseline.
Landing costs 300.03/300.04 ms per recorded frame, versus 303.84/304.40 ms.
All 82 player records, first-room/death pixels and palette match the baseline;
the phase-state and repeat gate passes. Both still miss 60 of 82 intervals,
retain the extra death-phase request, and fail item 15's 83.3 ms cadence gate.
These are exclusive PC measurements, not inclusive call-tree estimates.

The correctness-only run `.run-hwwlOQ` decodes all 82 publications against the
original Macintosh colour table and measured transfer, checking both field
palettes, plane pointers, display registers and terminators. All 81 successive
palettes are unchanged on that route; eleven altered-palette/input/pointer
controls fail. The separate native four-frame probe `.run-gVHGsE` covers an
actual palette-only update and viewport movement and passes all full-frame,
RGB24, field-pointer and restoration checks. The original-paired scene replay
`.run-6VLhmf` also matches the pinned Macintosh title and first-room indices,
all planes and both field palettes, including the palette transition.
Sanitizer tests cover 2,048
publications, palette changes before both lists catch up, key-only changes,
reset and unchanged reuse. The ordinary release remains free of the optional
`pop2CopperReady` observer.

A content-tile prototype was rejected. In `item15-tiles-020-a`, comparing and
updating a 196 KB chunky snapshot reduced phase-zero C2P from about 55 to
31 ms/frame, but added about 32 ms of planning work. Landing worsened to
315.40 ms/frame and the route took 662 fields. Its source and capture remain
ignored evidence; no tile cache is installed in the game. Inspection also found
that PaintRLE/PaintCopy already use assembly and wide copies; their remaining
loops are not equivalent to SaveRect's expensive compiler-generated loop.

`manifest/performance-68020-palette.sha256` pins the repeated measurements,
correctness captures, controls and rejected prototype. The native observer is
`amiga/copper_palette.gdb`; `tools/check_copper_palette.py` requires pinned
captures and positive route completion. Timing runs never enable its stops.

### Rejected row-end comparison pilot (item 15)

MEASURED: `item15-row-trim-020-a` and `item15-row-trim16-020-a` compare
screen CopyBits rows against their existing destination, copy the changed middle,
and form four- or sixteen-row dirty bands. Both use the calibrated fixed 68020.
Landing costs 293.19/292.79 ms, versus the accepted zone baseline's
295.06–295.36 ms. C2P falls to 31.58/36.76 ms from 54.78 ms, but CopyBits
rises to 29.28/29.32 ms from 18.46 ms and other presentation to 17.85/13.10 ms
from 10.21 ms. Route totals are 605/609 fields. These are different single-run
pilots, not repeated measurements of one accepted implementation.

Both complete the route and restoration, with all 82 player records and complete
first-room/death screen and first-room palette bytes identical to the accepted
zone baseline. The shared copy helper passes 8,192 sanitizer cases through
640-pixel rows. This does not establish intermediate display correctness:
the first pilot can suppress viewport-only publication; the second adds an
unqualified viewport-dirty guard. Neither has the full original-paired display
gates. The small net saving does not justify retaining the extra comparison and
planning path. The prototype is removed from both the port and AmigaXDev;
ignored sources and measurements are pinned in
`manifest/performance-68020-row-trim.sha256`. The accepted baseline is unchanged.

### Shared Kalms conversion kernel (item 15)

MEASURED: frozen-build `item15-kalms-final-020-b/c` take **275.99/275.91 ms**
per stored landing row, against the HUnlock baseline's 284.51/284.01 ms.
Conversion costs **49.98/50.01 ms**, against 54.77 ms in the latter baseline C.
Both final routes take **581 PAL fields**, preserve all 82 baseline player
records and endpoint images, and pass the repeat comparison with identical
ELF and executable bytes. First-room pixels and palette match the pinned
Macintosh reference. Both still have **59/82 late requested intervals** and
an extra death-phase request relative to the original; strict cadence fails.
The 83.3 ms request and item 15's acceptance are unchanged.

The port uses AmigaXDev's shared rectangle adapter and the unchanged Kalms
`c2p1x1_8_c5_030_2` kernel from the already pinned upstream revision. Vette and
Alone in the Dark were inspected first; the adapter follows Alone in the Dark's
row interface. No changes were made to those ports. The dirty planner, frame-end
conversion and publication schedule remain unchanged. Ordinary initialization
is used, never the upstream self-modifying initializer. The source screen has
four private padding bytes, and the adapter pads its scratch buffer, for the
kernel's unused final prefetch. Public PixMap dimensions remain 640×480.
Kernel, adapter and NDK include hashes are verified before assembly.

The standalone calibrated fixture `c2p-native-rhac0g4p` passes **193 cases**:
all 32-pixel widths through 512, four source alignments, one/two/seven-row spans,
word-aligned output offsets, full-screen and bottom-right extents, all C ABI saved
registers, unchanged source and complete output including surrounding guards.
The existing wrapper fixture launcher is reused. Its first compile failed on a
freestanding completion-string copy; no emulator ran in that attempt. The final
fixture uses a static string and returns an explicit completion record.

The final correctness capture `.run-5Nzwoe` passes **82 complete planar image
comparisons**, minimum dirty coverage, and byte-identical complete viewports
against the earlier kernel capture `.run-xk0vlH`. The latter also rejects 82
single-bit plane alterations. Six dirty-coverage controls pass. The checker
accepts an explicit publication workload; its default 81-publication contract
is unchanged. The current slow-020 workload explicitly requires 7/28/9/9/29
publications and does not qualify cadence. Host tests and the ordinary release
and link audits pass; original timing-service implementations are unchanged.

The initial kernel-only pilots measured 276.47/276.59 ms and passed repeats.
A separate assembly block-cache pilot measured 272.84 ms with 192 KiB extra
Fast RAM: conversion plus comparison/cache maintenance cost 45.93 ms, including
15.92 ms in its wrapper. It skipped over half the conversion blocks, but saved
only 3.63 ms against the kernel-only pilot. Its player records and endpoints
match; intermediate display fidelity was not qualified. The cache prototype is
removed, not shipped. Reconsider it only with a materially cheaper comparison
and cache-update structure; do not repeat this wrapper unchanged.

Reproduce the standalone fixture with `python3 tools/run_wrapper_fixture.py tmp
--kind c2p`, and the full-plane capture with `CALIBRATED_CPU=1 python3 tools/capture_c2p_bounds.py
--setup a1200-fast --replay FROZEN_BUILD`. For the recorded slow-020 route use
`python3 tools/check_c2p_bounds.py RUN --phase-counts 7 28 9 9 29`. Timings come from
separate uninterrupted `amiga/aprof.sh` captures. The reporter recognizes both
Kalms kernels in its exclusive conversion bucket. Inclusive call-tree costs
remain unverified. `manifest/performance-68020-kalms.sha256` pins the captures,
reports and rejected cache prototype; original-derived evidence remains ignored.

### Guarded HUnlock entry (item 15)

The native quick-entry table now handles valid HUnlock calls using the existing
owner, stack, frame, callback and deadline guards. A rejected handle leaves guest
state untouched and returns to the full dispatcher, preserving the original-paired
freed-handle caller policy and unknown-handle loud stop. The full and quick paths
share the same heap-state operation. Vette and Alone in the Dark were inspected;
the latter uses the same shared MacHeap state/setState operation. No independent
heap implementation is introduced.

MEASURED: frozen-build `item15-hunlock-native-020-b/c` take **284.51/284.01 ms**
per stored landing row and **589/589 PAL fields** for the opening route. Both
retain all 82 baseline player records and first-room/death images. Their repeat
comparison passes. The preceding native A run measured 281.53 ms, but A/B fails
the phase-zero field tolerance (71 versus 73); that failed comparison is retained
and the lower figure is not the qualified result. The earlier C-bridge pilot
measured 284.18 ms. The accepted register-copy baseline was 286.39/287.43 ms.

Both final runs still have **60/82 late requested intervals**, and the strict
original comparison rejects their extra death-phase frame/request sequence.
Item 15 remains open against the unchanged 83.3 ms request. Flat exclusive costs
in C include 78.21 ms original code, 65.60 ms trap entry/dispatch and 54.77 ms C2P;
unmatched returns preclude treating the printed call-tree costs as verified.

The final native fixture `.run-CxlaPu` passes 256 flag/CCR cases comparing the
actual native HUnlock body with the full service, 96 untouched-state invalid
handle rejections, and 32 original-paired freed-handle cases. The existing
`gameplay-hunlock-contract-1` Macintosh contract passes. Seven quick bodies pass
91 guard rejections and seven due-deadline rejections; 3,793 live guard checks
report zero faults. The callback and 28,388-copy fixtures also pass, and all
17 altered-evidence controls are rejected. The completed gameplay route restores
hardware and matches pinned original first-room pixels, palette, RNG and curated
context; native planes and copper match too. This is not a new exact-title-phase
comparison. Host tests and the restored ordinary release/link audits pass.
`manifest/performance-68020-hunlock.sha256` pins captures and reports, including
the rejected repeat and strict original-cadence comparison. Captures stay ignored.

### Shared saved-register copy (item 15)

The dispatcher uses Alone in the Dark’s `BlockMoveBytes` helper, published
unchanged through AmigaXDev, to move the 60-byte saved-register image when
Pascal argument cleanup changes the return stack. Both ordinary dispatch and
the callback shortcut retain overlap semantics. No original game routine,
rendering decision or scheduling guard changes. Exhaustive checks run only
under `POP2_QUICK_TRAP_PROBE`, before the game starts; the release audit rejects
their data and counter symbols.

MEASURED: calibrated `item15-register-copy-020-a/b` take 286.39/287.43 ms per
landing frame and 595/597 PAL fields for the route, versus 292.43/293.12 ms and
607/607 fields for the caller-classification baseline. The executable hashes
are identical across repeats; adding probe-only includes changed ELF debug line
tables. The phase-repeat gate passes, all 82 player records and endpoint images
match the baseline, and 59/60 of 82 timer intervals remain late. The strict
original delay comparison still fails because of the retained extra death-phase
frame. Item 15’s 83.3 ms cadence requirement is not met.

The profiler previously classified the generic `memmove` under CopyBits; its
replacement is inlined into dispatch. Bucket movement is not a rendering speedup.
The whole landing window supplies the before/after measurement above.

Verification route `.run-cfyfp4` passes 28,388 native copy cases, 64 callback
register/CCR cases, 128 callback rejections, 78 quick-guard rejections, six timer
deadline rejections and the unsupported-zone control. Its 3,286 live native
guard checks report zero faults. Fourteen altered-evidence controls reject
missing or incorrect results. CODE/input audits and hardware restoration pass.
First-room pixels, colours, RNG/context, AGA planes and copper palette/mode
match the pinned Macintosh reference. Host sanitizer checks and the full host
suite pass; `make release` passes the link and diagnostic-exclusion audits.
Evidence is pinned in `manifest/performance-68020-register-copy.sha256`.

### Original-caller classification (item 15)

Original CODE lives in separately allocated memory and cannot overlap the linked
native fixture routines. Once CODE ownership is verified, the dispatcher skips
the redundant native-fixture range comparisons. Other callers retain the full
classification and all stack, callback, patched-trap and pending-service guards.

MEASURED: calibrated `item15-dispatch-owner-020-a/b` record 292.43/293.12 ms per
landing frame, versus 295.06/295.36 ms for the zone baseline. Exclusive dispatch
cost is 65.27/65.54 ms versus 67.91 ms in baseline C. Both routes take 607 PAL
fields, retain all 82 baseline player records and endpoint images, and pass the
phase-repeat check. Both still miss 61 of 82 timer intervals; item 15 remains
open against 83.3 ms. The existing extra death-phase frame is not waived.

The verification route `.run-VtXyJL` passes the quick-entry/callback controls,
live guard checks, original CODE audit and restoration. First-room pixels,
colours, RNG/context, AGA planes and copper palette/mode match the pinned
Macintosh reference. The ordinary release link/probe audit passes.
`manifest/performance-68020-dispatch-owner.sha256` pins these captures.

An otherwise unchanged speed-compilation pilot (`item15-dispatch-o3-020-a`)
records 295.05 ms landing and 68.82 ms dispatch. It provides no useful saving
and is rejected; production retains its existing compilation policy.

### Zone services inside callbacks (item 15)

Alone in the Dark's specialized GetZone/SetZone entries were inspected first.
The port's existing user-mode callback shortcut now handles these two services
as well as its existing timer/fixed-point calls. It shares the zone bodies with
the full dispatcher, including the single-zone ownership check, MemError and
OS condition-code policy. Pending audio, an inactive callback or an invalid
caller retains the full path. Ordinary zone calls still use the full dispatcher;
there are no added native quick-table entries.

MEASURED: identical-build calibrated 68020 runs
`item15-zones-callback-020-c/d` complete the opening in 614/613 PAL fields,
against 624/622 before this change. Landing takes 295.06/295.36 ms per recorded
frame, against 300.03/300.04 ms. All 82 player records, first-room/death pixels
and palette match the prior build. The repeat phase-state and field-variation
check passes. These are exclusive-PC and completed-route measurements;
unverified inclusive call costs are not used.

MEASURED: final fixture `.run-QWccH0` passes both callback zone services with all
32 CCR inputs, preserved registers and untouched stack bytes: 64 positive cases
and 128 inactive/pending-audio rejections. It also passes the six ordinary quick
bodies, 78 guard rejections, six due-deadline rejections and the unsupported-zone
control. The route completes with 3,266 live guard checks, zero faults, CODE/heap
audits and hardware restoration. First-room pixels, colours, RNG, context,
AGA planes and copper palette/mode match the pinned Macintosh reference.
All thirteen altered reports are rejected. Synthetic cases explicitly
control the audio-readiness input without advancing or discarding the real
audio clock, engine or queues; the live route requires the override to be off.
This isolation is compiled only with `POP2_QUICK_TRAP_PROBE` and is absent from
release and profiling builds. The checker supports the selected CPU preset.
The host suite and final release/link audits pass; the release symbol audit
also confirms that the audio-readiness override and fixture counters are absent.

Cadence still fails: 60/61 of 82 requested intervals are late, and the extra
death-phase frame still fails the strict original timer-request comparison.
Item 15 remains open. `manifest/performance-68020-zones.sha256` records the
measurement and validation artifacts, including rejected experiments.

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

The current tested route has 58 updates and 58 conversion passes, 115 rectangles
and 826,944 converted pixels, with no conversion outside frame end. Benchmark
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

## Ordinary dispatch and clock validation (items 19.2–19.3)

Items 19.2 and 19.3 pass their service, integration, timing and measurement
gates on `a1200-030`. Ordinary release is restored and passes the diagnostic-
exclusion audit. Overall release and additional CPU validation remain open.

The ordinary entry uses indexed dispatch and native entries for SystemTask,
TickCount, HLock, HGetState, StripAddress and SwapMMUMode. Code-owner candidates
retain current range checks; segment installation invalidates the last validated
range. Per-service row candidates reread live base and size. Stack, unfinished
frame, parent and return-address checks remain. Failed guards and pending
services retain the full dispatcher and its named stops. Direct cached rectangle
entries remain separate from this ordinary trap path.

The scheduler uses the interrupt epoch or latest precise observation for
ordinary checks. TickCount and due VBL work retain precise beam sampling;
Time Manager samples when a deadline can fall in the current field. A reusable
no-due certificate expires at a changed tick or field and checks any current-field
microsecond deadline on every reuse. The full path invalidates it before queue
mutations. Full snapshots, ambiguous-position retries and backward-clock stops
remain. Shared PAL thresholds match the former arithmetic at every valid beam
position, all five phases and tick wraparound in host sanitizer tests.

The shared bounded 32-slot flag-address cache (`cb13b74`, Amiga type fix
`b2a5dc0`) retains only validated addresses, rereads mutable flags and clears on
arena reset. Slot collision/reset and full heap sanitizer tests pass. Allocation
and coalescing changes remain deferred to item 19.7.

MEASURED on the exact no-MMU `a1200-030` profile model, inclusive of scheduled
callback work:

| Trap | `dispatch-slot-map-b` µs/call | `dispatch-slot-map-c` µs/call |
|---|---:|---:|
| SystemTask | 13.597 | 13.464 |
| HGetState | 21.893 | 20.673 |
| HLock | 21.587 | 22.901 |
| StripAddress | 21.721 | 21.685 |
| SwapMMUMode | 22.510 | 22.524 |

Both captures contain 81 identical Kid records and match the original timer
requests in `opening-noright-1`. Phase state and repeat cadence pass. Player
records and per-phase frame work also match `dispatch-before-b`; before/after
cadence is assessed separately because fewer missed intervals change field
counts. Both after runs retain two late intervals on the exact no-MMU model.
The baseline pair `dispatch-before-b` / `-c` measured 35.46–35.70 ms/frame in
dispatch and 9.79–9.90 ms/frame in `pop2WallTicks`, with SystemTask at
80.6–82.9 µs/call. Those full-scene buckets are not D13 active-work measurements.
Per-phase `opening`, `report` and `work` output is retained under ignored
`tmp/dispatch-phase-reports/`.

The final native quick-entry fixture `.run-LYcGQf` completes the integration
route with 78 fixture calls, 72 rejected preconditions, six due-deadline
rejections and 134,209 live guard checks with zero guard or clock faults.
All nine altered reports are rejected and restoration passes. The following
release audit and final host suite pass. The complete core gate passes in
`tmp/core-gate-zeznhtmz`, paired with original
`20261015-193126-30ff96b7`: three native runs, seven runtime rejection controls
and five altered-evidence controls. The complete file gate passes in
`tmp/files-gate-9tgmugy2`, covering original-paired opening/save/restart,
resource payloads and metadata, native settings persistence and the installed-
asset write stop. Its five original file, three original persistence, seven
native persistence and eight resource rejection controls pass. The complete
display gate passes in `tmp/display-gate-jvler7_x`: paired service records,
four-frame presentation probe, two identical captured-scene replays with exact
client indices/AGA planes/RGB24 copper, ten evidence controls, three runtime
rejections and the 31-call startup regression through InitGraf. The complete
UI gate passes in `tmp/ui-gate-ydnbutv5`: nine original input keys/events,
two byte-identical seven-prompt captures, 144 paired startup calls, menu actions,
twelve evidence controls, four runtime rejections and the explicit startup
checkpoint. The complete audio gate passes in `tmp/audio-gate-ewj6hzgw`: 7,549
cutscene calls, two identical 9,329-call route replays, 1,135 Paula starts,
62 release tails and 16 callbacks per route replay, ten evidence controls,
six runtime rejections, the original observer byte-guard control and the
144-call startup regression. The following release audit passes.
The integration gate passes in `tmp/integration-gate-ekox1gnr` on `a1200-030`:
paired title rendering, two byte-identical gameplay capture sets, the no-Right
outcome control and 45 altered-evidence controls. The four original integration
service contracts also pass with ten rejected alterations. VBL (`.run-i6QPuB`)
and Time Manager (`.run-JK0z1Q`) match their pinned original fixtures and each
reject twelve alterations, including restoration checks. Cadence (`.run-weyO0i`)
matches 59 consecutive original five-tick intervals and rejects six alterations;
all 59 conversions occur at frame end. The following release audit passes.
The ordinary-trap rectangle fixture (`.run-fA3L9u`) passes 1,220 original/native
calls covering signed boundaries, empty/touching/overlap cases, input aliases,
CCR, data registers and Pascal stack/padding, with restored route completion.
EqualRect passes its 36 original cases and alias controls. Interrupted runs
supply no acceptance evidence.

The final scoped pair `dispatch-slot-map-active-a` / `-b` measures active
dispatch at **8.741 / 8.665 ms/frame** (limit 12) and inclusive clock sampling
at **0.688 / 0.631 ms/frame** (limit 2). Both captures match all 81 Kid records
and original timer requests, retain the same five phase workloads, and pass
repeat cadence against each other and the full profile. Both record 377 PAL
fields and two late intervals; the remaining exact-model opening cadence work
belongs to item 19’s overall gate. The executable and symbol hashes match the
full-profile pair. Both scoped captures reject seven altered/incomplete reports
and two accounting faults. Per-phase reports and the exact unrounded budget
audit are retained in `tmp/dispatch-phase-reports/` and
`tmp/dispatch-final-profile-audit.json`. The final `make release` passes link
and diagnostic-exclusion audits.

### Active-work capture scope (D13)

Set `APROF_ACTIVE=1` when running `amiga/aprof.sh`. The debugger verifies the
original LINK and UNLK/RTS instructions at `CODE 17+$31B8/$31DC`, records active
parts separately from each timer wait, and checks the matching stack and return
address at every exit. Exclusion runs from entry through UNLK, including
callbacks serviced inside that wait; its final RTS remains in active work.
Each phase boundary must be outside a wait. This measures the existing behavior
without changing wait or callback scheduling.

`report` sums active parts using the full scene's frame count and prints active
plus wait accounting. Clock sampling includes `pop2WallTicks`, `pop2ServiceTicks`
and named Time Manager queries, without nested double counting. That inclusive
subtotal overlaps the exclusive buckets and must not be added to their total.
Older captures with inlined Time Manager reads provide only a partial clock
subtotal and do not establish D13 clock acceptance.

Every profile reconciles flat PC time plus gaps to call-tree self time. The
profiler stop command leaves its final clock remainder unflushed; reports list
that tail separately and reject negative, misaligned or greater-than-one-colour-
clock tails. The checker requires balanced ordered waits, every part and all
five phases. `test_aprof_scope.py` rejects seven incomplete/altered captures and
two accounting faults. Apply it to the final scoped captures before acceptance.
Compare those captures with full profiles of the same executable; full-scene
profiles still supply per-trap costs and cadence.

`APROF_REPLAY` can name a frozen directory containing `PrinceOfPersia2.exe`,
`.elf` and `.json`. Both hashes and the exact opening-profile flags must match
the receipt before replay. Retain its source revision and original capture
identity. The before baseline is the frozen `dispatch-before-b` executable
from `4d6702f`; its scoped pair is `dispatch-active-before-a` / `-b`.

### Audio call cost (19.6)

The candidate reuses immutable instrument conversions through the shared bounded
cache, uses the existing shared span converter for mutable one-shot effects,
and records settled/stopped Paula state. Immediate replacements retain the
two-sample-period DMA-off wait but omit the intermediate DAC-zero handshake;
completed voices and hardware handoff still fully quiesce the DAC. Vette's
immediate restart and Alone in the Dark's retained samples/quiet-channel state
were inspected first. The audio and common gates pass on the combined working
candidate; the final dispatch acceptance is recorded above.

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

### Non-blocking buffer synchronization (19.4)

`PrinceScreen::field` starts the completed picture’s synchronization copies only
once the even-field swap has made the old front inactive. The existing shared
blitter queue runs those copies while the game works in Fast RAM; an owned BLIT
completion handler advances queued jobs. The next conversion drains outstanding
work before touching the destination. File-service windows and shutdown quiesce
publication and copying before restoring the OS blitter vector. Prepared large
scenes retain their completed-copy boundary. The bounded batch fits the existing
queue; no extra bitmap, pixel scan or release diagnostic is introduced.
Because the next frame’s dirty regions are not yet known, this anticipatory copy
cannot skip bands that the next conversion will replace completely. The landing
scope submits twelve rectangles rather than seven; the measurements include
that extra traffic. Other scenes remain subject to item 19.8’s profiling gates.

Vette’s dirty-buffer ownership and Alone in the Dark’s blocking blitter path were
inspected first. The queue and copy engine remain AmigaXDev’s existing framework.
The reusable completion policy is canonical `BlitterCompletion.h`, published as
`1dcd44e` on `codex/blitter-completion`, installed and vendored byte-identically.
AmigaXDev’s unrelated dirty main tree is unchanged; integration is retained in
19.9. No installed source was edited in place.

MEASURED: two exact-cycle/no-MMU `a1200-030` baselines (`sync-before-c/d`, parent
`f2d667b`) and two candidates (`sync-after-a/b`). Every run has `opening`, `report`
and `work` output for all five phases. Both within-build repeat checks pass.
The following numbers retain the existing **reported phase-frame denominator**
(six landing rows/seven publications); inclusive costs overlap.

| Phase | Entire drain, before (ms/frame) | Entire drain, after | Active work, before | Active work, after |
|---|---:|---:|---:|---:|
| 0 landing | 13.997–14.006 | 0.00509–0.00510 | 96.92–97.24 | 81.77–82.55 |
| 1 stand-off | 3.127–3.298 | 0.00478–0.00480 | 43.83–44.00 | 40.43–40.64 |
| 2 attack | 5.388–5.747 | 0.00413–0.00446 | 44.69–45.94 | 39.13–39.36 |
| 3 attack | 2.498–2.902 | 0.00472–0.00488 | 43.37–43.81 | 40.73–40.74 |
| 4 fall | 1.432–1.552 | 0.00465–0.00484 | 26.06–26.82 | 25.42–25.58 |

The ≤2 ms landing gate passes even when the **entire drain subtree**, including
queue polling and call overhead, is charged as CPU waiting. The hardware-wait
leaf itself falls from 13.991–14.000 to 0.00189–0.00206 ms/frame. Inclusive landing
publication falls from 33.21–33.50 to 19.36–19.38 ms/frame. VBI display work grows
from 0.112–0.113 to 0.267 ms/frame, including enqueuing; completion-handler
instructions add about 0.010 ms/frame exclusive of helpers and Exec dispatch.
The active-work totals include the deferred queue and handler code. Unattributed
exception/STOP gaps are reported separately: 2.90–2.92 ms/frame before and
2.96–2.97 after. Whole landing-scope CPU time falls from 99.78–100.53 to
93.39–94.19 ms/frame, with more time available for the original timer wait.
The landing saving therefore survives accounting for the added queue/handler
work. Initial scene preparation and final shutdown quiescence are outside this
bounded opening window; no startup or quit-speed improvement is claimed.

Both candidates complete all **81 original requested timer intervals exactly**,
with no early or late interval; both baselines have one one-tick overrun. Total
opening duration is 375 PAL fields versus 376. All 81 frame counters, phase IDs
and complete captured Kid records match each paired baseline, as do requested
delays from `ref/mame/runs/opening-noright-1`.

The strict repeat comparator is unchanged. Applying it across builds rejects
`sync-after-b` because landing falls from 25 to 23 fields, beyond its symmetric
one-field repeat tolerance; that failed report is retained. The separate
`check_blitter_profile.py --compare` gate requires identical work, exact original
timer intervals and no phase slowdown beyond the existing quantization allowance,
while allowing a faster phase. Its six controls reject altered state, frame
count, requests, early/late intervals and a slowdown. The old blocking build
fails the 2 ms gate; a zero-budget control also rejects the candidate.

Reproduce the budget and workload check after collecting the normal reports:

```sh
. amiga/env.sh
python3 tools/check_blitter_profile.py sync-after-a --compare sync-before-c --controls
python3 tools/check_blitter_profile.py sync-after-b --compare sync-before-d --controls
```

The shared sanitizer tests cover 8,481 batch/latency combinations through the
256-job bound, including immediate completion and early drains. Three mutated
policies (acknowledging after starting, disabling completion interrupts, omitting
the drain) fail. The four-frame native fixture (`.run-njWKFB`, `a1200-030`) passes
all decoded pixels, palettes, viewport movement and restoration; it records five
completion interrupts. Missing/zero completion records, a damaged preserved
plane and a failed vector restore are rejected. Both VERTB and BLIT vector
identity now contribute to the existing restoration guard.

The separate full-plane capture (`.run-Br0bsP`, ordinary `a1200-030` with MMU)
completes all 81 publications. Every full viewport is byte-identical to the
pinned pre-change capture `.run-ypgCZO`; independent chunky/plane decoding and
minimum conversion bounds pass on every frame. These debugger-stopped captures
are correctness evidence only, never timing measurements.

The common integration gate (`tmp/integration-gate-ujb8ai2a`) passes the title
state, natural-route repeat and no-input control (`.run-HP8dlA`, `.run-4I0iGf`,
`.run-KSXVJP`, `.run-OeUAJ9`), including reference pixels/state and restoration.
Its 20 title-native, six title-reference and 19 evidence controls pass. The paired
original service contracts and ten alteration controls pass. VBL (`.run-3BsbMw`)
and Time Manager (`.run-K6mfjD`) pass their original contracts and twelve
alteration controls each. Host tests and the shared sanitizer suite pass.
Cadence (`.run-5YMqJG`) passes all 59 consecutive original five-tick intervals,
six cadence controls and 16 performance controls. All 59 route updates have one
conversion pass at frame end. Window-end and route-end screen/context hashes
match `manifest/performance-parent.sha256`. The ordinary release is restored and
passes its unchanged link/release audit, including absence of the new diagnostic
interrupt counter. `manifest/blitter-sync.sha256` pins the measurements, native
captures, gate results and frozen release build. These are emulator results;
item 14 retains hardware validation and archive refresh.

### Conversion bounds (19.5)

`PrinceScreen` uses the coordinate-only disjoint-band planner extracted from
Alone in the Dark into AmigaXDev (`16586b1`, `codex/dirty-bands`). Vette's
completed-frame conversion and buffer synchronization were also inspected.
The planner unions clipped draw coordinates before Kalms alignment, coalesces
identical adjacent bands, and retains a bounded bounding-box fallback. It reads
no pixels and adds no startup verification or release diagnostics.

MEASURED on `a1200-030` with the same exact-cycle/no-MMU profile configuration:
`bounds-before-c` / `bounds-before-d` versus `bounds-after-a` / `bounds-after-b`.
Both repeats and both before/after comparisons pass with 81 identical Kid records
and original timer requests. Each capture has `opening`, `report` and `work`
outputs for every phase under ignored `tmp/aprof/`.

| Phase | C2P inclusive, before | C2P inclusive, after | Publication inclusive, before | Publication inclusive, after |
|---|---:|---:|---:|---:|
| 0 landing | 18.50–18.51 | 17.36–17.45 | 34.52–34.60 | 33.40–33.51 |
| 1 stand-off | 6.77–6.82 | 6.49–6.54 | 12.08–12.18 | 11.28–11.49 |
| 2 attack | 5.90 | 4.74–4.86 | 12.46–12.79 | 11.67–11.96 |
| 3 attack | 5.09–5.36 | 4.62–4.79 | 9.18–9.69 | 8.72–9.32 |
| 4 fall | 4.67–4.87 | 4.60–4.86 | 7.57–7.69 | 7.59–7.75 |

Numbers are ms per reported phase frame, retaining the established report's
row denominator. The landing boundary fires after publication but before storing
the next phase's row: phase 0 contains seven publications and six stored rows;
the last phase contains 28 publications and 29 rows. Thus the phase-0 inclusive
C2P cost per actual publication is 15.86–15.87 → 14.88–14.96 ms. This boundary
accounting is unchanged between arms; no report is relabelled. Exclusive C2P
measures 17.79–17.80 → 16.65–16.91 ms per reported phase-0 frame. The inclusive
reduction is about 6%, not the 30% branch of the acceptance gate.

The exact no-MMU opening has one late requested interval in both candidate
runs, against two in both baselines, with 376–377 versus 377 total PAL fields.
The later 19.4 measurements above resolve that remaining interval. These are
emulator observations, not hardware performance claims.

The separate ordinary-emulator bounds runs are `.run-kjl4g5` (before) and
`.run-ypgCZO` (after). Each completes the no-input route and restoration and
passes 81 full-frame chunky/bitplane comparisons. All 81 before/after viewports
are byte-identical. The independent row optimizer considers every legal
16-pixel-aligned interval of width divisible by 32; every candidate publication
has minimum area and no overlap. This establishes minimum coverage of the
original dirty coordinates, not that every pixel inside each original rectangle
changed. No framebuffer shadow or pixel scanning is needed in the game.

The landing scope's seven publications convert 285,184 → 271,840 pixels
(4.68% less); the complete route converts 1,217,792 → 1,172,992 (3.68% less).
The first six publications were already minimal; the landing publication and
ten later updates had redundant coverage. **19.5 uses the minimum-bounds
acceptance alternative**, not a claimed 30% timing reduction. The planner's
release stack frame is 820 bytes and publication's is 604 bytes, before outer
service frames and interrupts, within the existing 16 KiB program stack.

Host controls cover 8,192 independently optimized row patterns across both
ports' viewport sizes, 10,000 extraction-identity cases against Alone in the
Dark's planner, clipping and capacity fallback, and 400 alternating-buffer
updates with complete plane preservation. Six malformed/missing/overlapping/
nonminimal coordinate controls are rejected. Captures remain ignored;
`manifest/c2p-bounds.sha256` pins the measurement and native evidence.

Reproduce the correctness capture separately from timing:

```sh
. amiga/env.sh
python3 tools/capture_c2p_bounds.py
python3 tools/check_c2p_bounds.py NATIVE_RUN --compare BEFORE_RUN
python3 tools/test_c2p_bounds.py
```

The four-frame AGA fixture passes in `.run-jxzgEE`; the full host suite and
`make integration-check` pass in `tmp/integration-gate-4dlowfhg` on
`a1200-030`, with four fresh Macintosh service comparisons, ten contract
controls, 20 native-title controls, six reference-title controls, 19 gameplay
evidence controls, and the completed no-Right outcome control. VBL
(`.run-nZX01L`) and Time Manager (`.run-M6BgsH`) match their original contracts
and each reject twelve altered-evidence controls. Cadence (`.run-FZh0IY`)
matches 58 consecutive original five-tick intervals and rejects six alterations;
its performance checker rejects sixteen altered window/route/parent records.
All 58 conversion passes occur at frame end. The restored ordinary release
passes the link and no-runtime-hash/trace audits.

The capture's debugger stops invalidate its timings. The unchanged
`amiga/aprof.sh` route supplies the measurements above; run both repeats with
`--original ref/mame/runs/opening-noright-1` and produce `report` and `work`
for phases 0–4. The shared planner and its tests are published on AmigaXDev's
isolated branch; integration into AmigaXDev's main branch and adoption by Alone
in the Dark remain in 19.9.

## Scenes to retain for future profiling

Record suspected heavy scenes encountered during gameplay validation here,
including the level/room or cinematic, reproducible route or save, observed
symptom, target/build and whether a matched-workload measurement exists.
Debugger waits and verification-run wall time alone do not establish a slowdown.
The additional cinematic measurements are described below.

The level-five carpet exit is a transition workload. The corrected profiling
route `scenes-carpet-full` explicitly exercises NIS 1 before entering level six;
the earlier short assisted route consumes that transition with copy protection. `Profile 05` supplies a level-entry starting point, not a save
immediately before the carpet. Retain it as a comparison workload when measuring
any suspected transition slowdown.

The level-eight throne-room dissolve is measured below. The original
generates/persists dissolve data when its cache is absent; retain both a clean
cache and an already generated cache for comparison. The pinned clean route in
`manifest/gameplay-assisted-level8-clean.sha256` and its visible cutscene captures
in `manifest/gameplay-throne-reference.sha256` reproduce the scene. The new
release-derived profile uses `a1200-030`; debugger and verification wall times
are not performance measurements.

### Heap allocation and local resize (19.7)

The allocator uses AmigaXDev's `HeapResize.inc` and `HeapMoveHigh.inc`.
In-place resize merges only its newly split tail instead of rescanning the
whole zone. MoveHHi retains the original's free-tail placement and compacted
neighbour behavior, including pinned and purgeable blocks. A fresh System 7.5.5
fixture (`heap-contract-4`) covers nine placement cases plus unchanged-size,
shrink, growth and locked low-memory failure; host fixtures pair those outcomes.
Reproduce with `POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1 POP2_HEAP_PROBE=1`
on `tools/run_reference.py --name NAME`, then run
`tools/check_heap_contract.py ref/mame/runs/NAME --controls`. The fixture is
opt-in and uses its own private Macintosh heap zone.

NewHandle and NewPtr retain their existing logical clear extent and use the
already shared `FillRect8::fillSpan` longword stores. No game-visible bytes are
left uncleared to obtain a saving. `HeapFindSpace.inc` skips only a known occupied
prefix, preserving first-fit order even when the first free block is too small.
The hint occupies one 32-bit word outside the guest arena. Split rewinds it,
coalescing records the first free block, and compaction resets it. The diagnostic
whole-heap invariant checks that it names a block boundary with no free block
before it. No slot order, error guard, heap size or release verification policy
changes.

The helper is published in AmigaXDev's `codex/heap-search-start` (`b09d897`),
with canonical, installed and vendored sources identical. Shared sanitizer
checks cover 2,555 small occupancy/hint/request cases; the consumer checks
20,040 first-fit queries across 2,500 mixed operations and explicit shrink,
compaction and reset cases. Removing each of the split, coalescing and
compaction updates is rejected. Dirty-reuse clearing tests cover empty, short
and long allocations, padding and neighbouring payloads. The shared resize and
MoveHHi sources are pinned at `4ef7305` and `2fcb917`. Vette uses Exec allocation;
Alone in the Dark supplies the common arena allocator and original fixture.
Its remaining clearing/search work is recorded as HEAP.3 in that project's queue.

MEASURED: fresh parent `02e3c20` runs `heap-before-c/d` and candidate
`heap-final-a/b` use `amiga/aprof.sh`, `a1200-030`, exact cycles, no MMU,
`PROFILE=1 VERIFY=0` and the existing opening recorder. Each has `opening`,
`report` and `work` reports for all five phases, compared with
`ref/mame/runs/opening-noright-1`. Both pairs pass repeatability. Before/after
comparisons retain all 81 Kid records, counters, phase states and original
requests; both final runs meet all 81 timer intervals exactly. The complete
opening takes 375 PAL fields in both final runs (baseline 375–376). Measurements are emulator results, not hardware
claims or startup/scene-loading benchmarks.

Landing uses the established six reported rows/seven publications denominator:

| Quantity | Before, two runs | After, two runs |
|---|---:|---:|
| NewHandle, seven trap calls, inclusive ms/call | 4.012–4.208 | **0.453–0.455** |
| NewHandle inclusive ms/reported frame | 4.681–4.910 | 0.528–0.531 |
| Allocate, twenty calls, inclusive µs/call | 251.6–252.4 | 17.88 |
| Whole-zone coalescing, calls per landing phase | 28 | 14 |
| Coalescing inclusive ms/reported frame | 1.708–1.839 | 0.856–0.941 |
| Entire phase CPU ms/reported frame | 94.25–94.40 | 92.53–93.90 |
| Exception/STOP gaps, separately reported ms/frame | 3.00 | 2.88–2.92 |

The NewHandle budget includes the full trap subtree, including dispatch and
nested callbacks; it is not an average diluted with tiny internal allocations.
`tools/check_heap_profile.py` requires all seven calls and ≤0.5 ms/call, rejects
the baseline and the initial clear/resize-only candidate (0.701 ms/call in `heap-after-a`), and
rejects four missing-work/over-budget controls. Inclusive rows overlap and must
not be summed. The retained clear/resize-only pair is `heap-after-a/b`.

Work outside the original timer-wait subtree, ms/reported frame:

| Phase | Before, two runs | After, two runs |
|---|---:|---:|
| 0 landing | 81.98–82.38 | 77.47–77.84 |
| 1 stand-off | 40.45–40.74 | 39.60–39.81 |
| 2 attack | 39.39–39.40 | 37.51–38.17 |
| 3 attack | 40.25–40.70 | 38.99–39.73 |
| 4 fall | 24.89–25.39 | 24.26–24.61 |

These are matched `work` reports under the existing call-tree accounting, not
new D13 scoped clock/dispatch measurements. Faster work leaves more time in the
original wait; original requested cadence and gameplay decisions are retained.

Validation on `a1200-030`: `integration-gate-i8jvujoe` passes the title-state,
natural route, repeat and no-Right control, including all existing evidence
controls. `gameplay-restart-gate-5oryb2xi` and
`gameplay-save-gate-iqtxfntv` pass two native runs each, with 15 restart and 22
save/load rejection controls. Freed-handle run `.run-uL7oeU` reaches the
supported disposed-palette path once, matches all four original expired-story
checkpoints and restores normally. Its checker rejects zero/missing hits and
missing completion; the original isolated, live-scene and ending handle
contracts remain pinned and pass. VBL `.run-qWISIc` passes the original
countdown/removal and deferred-callback contracts, ABI, restoration and all 12
rejection controls.
Time Manager `.run-CJBHtM` passes delay units, active prime, cancellation, zero
delay, rearm, user-mode ABI and restoration, including 12 rejection controls.
Cadence `.run-oPtM2V` passes all 58 consecutive original five-tick intervals
and six rejection controls. Its five-update window is 18 PAL fields (72
ms/update); all 58 conversions occur once at frame end. This instrumented
window is separate from the full opening profile above.

Release `.run-O2eZ1P` with 4 MB Fast RAM positively reports `CORE / MEMORY`,
without an exception, at preparation stage 1 before hardware takeover. It is an
expected launch rejection, not a completed/restored gameplay run. Six controls
reject altered build/configuration, stop reason, exception, stage and completion
records. The original private-zone fixture also confirms failed locked growth
returns `memFullErr` with its address, old size and payload preserved.
The ordinary 8 MB release `.run-5LNhfV` reaches the original first-text
checkpoint with identical full viewport pixels. `make release` restores the
ordinary flags and passes the HUNK/link and release audits: no runtime hashing,
trap trace or hash counters. `manifest/heap-allocation.sha256` pins the original
fixture, all six profiling runs (including the rejected clearing-only pilot),
common-gate evidence, controls and frozen release binary. Build receipts match
within each accepted profiling pair; release output matches its tested receipt.


### Further cinematic scenes (19.8)

The opt-in scene measurement uses the exact no-MMU `a1200-030` model,
50 MHz, 8 MB Fast RAM and PAL, with `PROFILE=1 VERIFY=0`. It starts at the
marker within the first full service dispatch inside the selected NIS and ends at its natural
return. The prefix before that service is outside the scope. The
return wrapper preserves registers, stack and condition codes; timer and
adaptive-dissolve observations are buffered in Fast RAM. No debugger stop
occurs within the measured scene. The subsequent named `SCENE PROFILE /
COMPLETE` stop restores hardware and returns DOS failure deliberately; it is
not ordinary gameplay completion or a claim that the whole level was tested.

The carpet input repeats the successful level-five route with one logged D6
copy-protection acceptance write. Without it, protection selector 100 consumes
the exit cinematic. `Profile 05` alone is only a level-entry save. The throne
input repeats the accepted level-eight approach. Clean cache has DSLV 6400
only; generated cache adds the pinned original-generated DSLV 6401 resource
fork, retaining the baseline data fork and Finder metadata. Neither mode skips
the original's calibration. Installation keeps displaced inputs in `.replaced/`.

The paired Macintosh gate requires original identity, packing/copy branch,
seed, opening pixels, completed discovery and the unique selected NIS return.
It compares the return viewport and palette, actor/RNG state and timer requests.
It keeps the unnormalised startup trap trace separately: generated-cache loading
changes polling/callback interleaving, so this scoped gate does not replace or
weaken the generic startup-trace pin. The nonaccepted `scenes-throne-warm-reference` full-route replay fails
its post-cinematic checkpoint; that is retained separately and is not relabelled
as a level-eight route pass.

Throne uses nine SetTimer requests at distinct transition sites, not a repeated
frame timer. Its six DrawDisolveData calls request 240, 180, 20, 20, 20 and 180
ticks. Original and native execute 240, 188, 20, 20, 20 and 188 adaptive drawing
quanta respectively. These quanta are not display frames: the original compares
TickCount with its start plus progress and adjusts its per-word delay. Reports
state lateness against that original deadline separately from transition time. The
Macintosh observer samples the low-memory tick at TickCount entry; the native
probe buffers its returned tick. A tick boundary during the trap can affect
one-tick comparisons. This is a profiling observation, not an exact frame
equivalence or an uninstrumented-release cadence claim.

Flat instruction-PC costs include original polling and idle callbacks. The long
cinematic call tree has unmatched returns and is not suitable for inclusive or
active-work attribution; no D13 dispatch-budget claim is made from these totals.
The report retains those diagnostics for the shared parser work in 19.9. Timer
boundary intervals, loading, calibration and complete-scene duration are not
labelled frame cadence. These are emulator measurements, not hardware claims.

Reproduce with `tools/capture_scene_reference.py NAME carpet|throne` (add
`--cache generated` for the warm throne reference), then
`tools/aprof_scenes.py run NAME carpet|throne` with the same cache choice.
Pin fresh reference output names and SHA-256s in
`manifest/scenes-reference.sha256` before pairing. Use
`tools/aprof_scene_report.py NAME --compare REPEAT` and
`tools/aprof_scenes.py pair NAME ref/mame/runs/REFERENCE` for matched workload,
original return and deadline checks. Runs refuse existing output names.


The generated-cache pair skips one of the two MakeDisolveData calls; both modes
still perform six calibration calls and six draws. DERIVED from InitDissolve:
selector 2 deliberately bypasses cache load/save, so one generation remains even
with DSLV 6401 installed. The measured scope takes 3,761/3,768 PAL fields clean
and 3,787/3,775 with that cache (75.22–75.74 seconds). This does not establish a
whole-scene cache speedup. These totals include original polling and calibration; a cache-generation saving must not be inferred from its overall length.

The first 240-tick dissolve misses 89/87 of its 240 quantum deadlines in the
clean pair, by at most 15/16 ticks, and 87/89 with generated cache, by at most
16/15 ticks. The remaining five draws have no late quanta in these native runs.
The paired original clean reference has 57 late quanta in the first draw, at
most one tick; its generated-cache reference has 101, at most two ticks.
The original's three short 20-tick draws also have some one/two-tick misses.
Therefore neither total duration nor the presence of any missed quantum alone
establishes a port regression. Item 19.10 retains the larger first-dissolve lag
for focused comparison, including the original adaptive behavior.


The carpet reference completes three Mohawk animations of 118, 80 and 84
frames. The native flat-PC counters check three animation returns and 282 frame
increments, alongside the full return viewport and actor/RNG state. There are
zero SetTimer calls inside this NIS; the original observer's opening scene and
the measured throne provide positive controls for that recorder. This is not
a zero-delay or a passing frame-cadence result. Mohawk's per-frame deadlines
remain separate follow-up work in 19.10. The corrected native pair measures
2,929/2,931 PAL fields (58.58/58.62 seconds); its first run’s largest flat category is presentation
outside C2P (20.864 seconds), followed by dispatch (12.178 seconds). These
categories include any polling/waiting in those routines; they are not all
removable work.


| Scene/cache | Native repeats | PAL fields | Original-paired workload |
|---|---|---:|---|
| Carpet, clean | `carpet-e`, `carpet-f` | 2,929 / 2,931 | Three animations, 282 frame updates; no dissolve work |
| Throne, clean | `throne-clean-c`, `throne-clean-d` | 3,761 / 3,768 | Nine timer requests, six draws, 676 adaptive quanta |
| Throne, generated | `throne-warm-c`, `throne-warm-d` | 3,787 / 3,775 | Same timer/drawing workload; one fewer generation |

Each pair has identical executable/ELF receipts, curated entry state, requested
work and curated return state, with measured scope within 2%. The scene checker requires
positive natural return, the named completed stop, and restored hardware/input.
Twenty-two altered-evidence/workload controls reject changed workload, build
identity, clock signature, completion, machine, truncated records and wrong
clock sites; the observer control also excludes runtime writes and per-frame
debugger stops. The zero-timer carpet
pilot `carpet-c` is rejected because its recorder attempted an empty memory dump; corrected
runs explicitly record zero rows. The GDB-write boundary pilot `throne-clean-pilot` is also
rejected. Neither pilot supplies the accepted timings above.


The common opening baseline (`scenes-before-c/d`) and post-change pair
(`scenes-after-c/d`) each cover 81 frames and 375 PAL fields. All four meet all
81 requested intervals from `ref/mame/runs/opening-noright-1`; repeats retain
all 81 identical Kid records and phase/stall boundaries. Each run has `opening`,
`report` and `work` output for every phase 0–4. The full host suite and the
scene report/assistance controls pass.


`make integration-check` passes in `tmp/integration-gate-jdbe6x2l` on
`a1200-030`: title state, two natural-route runs and the no-Right outcome,
including all existing altered-evidence controls. The four service-contract
references are fresh (`20261017-004211-0b1e62c9`, `20261017-004304-a362556d`,
`20261017-004350-ab006073`, `20261017-004519-d2d7f9f4`) and their ten
rejection controls pass.

VBL `.run-LgXmfk` matches the original countdown/removal and deferred-dispatch
contracts, user-mode ABI and restoration, with all twelve altered-evidence
controls rejected.

Time Manager `.run-2Qev0D` matches delay units, active prime, cancellation, zero
delay, rearm, user-mode ABI and restoration, including twelve rejection controls.
Flat entry counters independently match the original’s two copy operations and
zero text pages for carpet, and one copy/nine text pages for throne.

Cadence `.run-CAjYI8` passes 59 consecutive original five-tick intervals and six
rejection controls. Its separate five-update window is 18 PAL fields
(72 ms/update); all 59 conversions occur at frame end. The performance checker
also passes its altered-evidence controls.

The restored ordinary release `.run-vMBOus` reaches the original first-text
checkpoint with matching full viewport pixels. `make release` passes the HUNK,
link and ordinary-build audits, with no runtime hashing, trap trace or hash
counters. Its executable is byte-for-byte identical to the pinned 19.7 release
(`c913cb3947c50d1cfe99297bec4636953ce9a7b93b7236101cdd62f8c6641247`).
`manifest/scenes-reference.sha256` pins the paired original scene evidence;
`manifest/scenes-profile.sha256` pins the six native scene measurements, four
opening runs, common gates, controls and frozen ordinary release. The four
large service-contract captures are pinned through their directory-local
`contract-evidence.sha256` indexes; verify both the tracked manifest and those
indexes to check every captured file. Captures,
profiles, binaries and reference disks remain ignored. No ordinary game-service
implementation or original game decision changes in this item.


### Cinematic deadline investigation (19.10)

DERIVED from CODE 15+$3222–$3248: the dissolve adjusts its per-word busy-loop
count by one after each quantum. Being ahead increases that count, being late
reduces it unless it is already zero, and an exact tick leaves it unchanged.
The target is the initial TickCount plus the quantum index. These are feedback
targets, not presentation timestamps. The initialization at +$309C–$30B4
clamps the initial count to 0–100; subsequent feedback can exceed 100.

MEASURED in the pinned 19.8 records: all 670 consecutive feedback transitions
per native run agree with that rule in `throne-clean-c/d` and `throne-warm-c/d`.
Both pinned Macintosh references agree too. The native first draw starts with
count 100, runs 15–16 ticks ahead, increases its count above 230, then runs
15–16 ticks behind while reducing it again. The Macintosh starts with counts
46/48 and stays within one/two ticks of target. This establishes feedback
oscillation in the observations; it does not establish how an altered algorithm
would perform, or authorize changing the original game algorithm.

`tools/cinematic_deadlines.py NATIVE REFERENCE` verifies the relevant input
pins and checks every feedback transition, reporting signed lag rather than
only late quanta. Its controls reject broken transitions, missing progress and
empty observations. The other five draws are often far ahead of their targets
on both systems; zero late quanta must not be described as exact cadence.

DERIVED for carpet: CODE 16+$0366 tests an unsigned caller TickCount against
object+$0E. Its initial-poll flag first sets that target to the caller's tick.
After an eligible update, +$0454 rearms it from that same caller tick plus the
signed delay at object+$0C, which the decoded animation step can change.
Thus the schedule advances from the eligible poll, not from a global SetTimer
or a fixed whole-scene frame rate. Two completed original observations (`deadlines-carpet-original-b/c`)
record identical workloads and timing summaries: 119, 81 and 90 eligible updates for the three animations, including
updates that do not draw a frame. Each rearms with a six-tick delay after decoding;
the initial stored delay is three ticks. The original itself sometimes reaches
an eligible update much later (up to 242 ticks). This is an earliest-update
threshold, not evidence that every displayed frame must meet a six-tick upper
bound. The existing Alone in the Dark and Vette! profile tools were inspected first;
the object fields and feedback rules here belong to this game, while the existing
profile parser remains reused.

MEASURED: final native repeats `deadlines-carpet-native-c/d` execute the same
119/81/90 eligible updates, three animation returns and 282 drawn frames as both
original captures. All requested delays and object-frame transitions match.
Both native runs positively return from the scene, restore hardware, and match
the original return viewport, palette and curated actor/RNG state. The repeated
scene scopes span 2,926/2,927 PAL fields (58.52/58.54 seconds).

| Carpet animation | Original lag p50 / p95 / max (ticks) | Native repeat C | Native repeat D |
|---|---:|---:|---:|
| First, 119 eligible updates | 0 / 0 / 242 | 8 / 9 / 224 | 8 / 9 / 226 |
| Second, 81 eligible updates | 3 / 5 / 33 | 0 / 1 / 32 | 0 / 1 / 30 |
| Third, 90 eligible updates | 0 / 0 / 202 | 0 / 0 / 308 | 0 / 0 / 312 |

Lag is sampled at the eligible poll against the object's requested threshold;
it is not the time at which pixels become visible. The first animation is a
particularly heavy scene on the measured 68030 and is not cadence-equivalent
to the Macintosh. The other two do not erase that difference. Large outliers
remain in the table; the report does not remove them or label the whole scene a
passing six-tick frame loop. The original algorithm and ordinary game services
are unchanged by this investigation.

The explicit `POP2_SCENE_DEADLINES` observer wraps the existing Mohawk due-check
method, preserves registers/CCR and its RTD stack cleanup, buffers eligible
updates, and restores the original method-table slot at scene return. It does
not change object clocks or request extra TickCount calls. Native profiling has
no debugger stop inside the scene. Flat self time in its wrappers and recorders
is 1,924/1,888 ms across the complete scene, excluding called helpers; this is
not an inclusive overhead estimate and is not subtracted from cadence. These
are instrumented observations, not an uninstrumented-release performance claim.
The ordinary release executable remains byte-identical to the existing release
in the completed audit.

Reproduce original captures with `tools/capture_scene_reference.py NAME carpet
--deadlines`; `manifest/cinematic-deadlines-reference.sha256` pins both completed
references. Use `tools/aprof_scenes.py run NAME carpet --deadlines`, then the
existing scene report/repeat and visible/state pair checks. Finally run
`tools/carpet_deadlines.py REFERENCE --native tmp/aprof/NAME` to check the
original delay/rearm rule and exact eligible-update workload. Controls reject
missing completion, damaged records, changed delays, broken rearming, missing
progress, altered result counts and changed workloads.

The earlier `deadlines-carpet-native-a` trial also completed and paired; the
final pair adds immediate named stops on invalid observer state. Attempt B
failed conversion before emulator launch because its diagnostic error branch
used an unsupported PC-relative relocation; the final wrapper uses a supported
absolute jump. The sandbox-blocked original attempt A never booted and is not
an accepted capture.

The two opening baselines (`deadlines-before-a/b`) and two post-change runs
(`deadlines-after-a/b`) each meet all 81 original timer intervals and span 375
PAL fields. Both repeat pairs preserve all 81 Kid records and phase/stall
boundaries. Each run has opening, report and work output for phases 0–4.
The host suite passes. Fresh fixed-point, font, GWorld and title-copy references
(`20261017-125932-c2d2ea8b`, `20261017-130029-b17bc6bb`,
`20261017-130119-ab70f857`, `20261017-130249-0a1ccca7`) pass their contracts
and ten rejection controls. VBL `.run-MtGfLo` passes the paired stable-sample
contract, user-mode ABI, restoration and thirteen rejection controls.
Time Manager `.run-wKb7gk` passes its original-paired delay, rearm, cancellation,
user-mode ABI and restoration checks, including twelve rejection controls.
Cadence `.run-FK7h5v` passes all 59 original five-tick intervals and six rejection
controls. Its five-update window spans 18 PAL fields (72 ms/update including
pacing); all 59 conversions occur at frame end.
The full integration gate `tmp/integration-gate-vw1mveqn` passes the title-state
comparison, two natural gameplay routes, and the no-Right control (same first
room, zero Right reads, different completed outcome), with 45 altered-evidence
controls. Every native route completes and restores hardware. `make release`
restores the ordinary build with SHA-256
`c913cb3947c50d1cfe99297bec4636953ce9a7b93b7236101cdd62f8c6641247`,
byte-identical to the pre-change release; its audit excludes runtime hashing
and tracing. Build controls reject the deadline observer outside its explicit
scene harness.

`manifest/cinematic-deadlines-evidence.sha256` pins completed captures and
reports. The four fresh service-contract references have directory-local
`contract-evidence.sha256` indexes: verify those files as well as the tracked
index to check every captured input. Captures and original-derived material
remain ignored.

## Shared profiling and debugger observations (item 19.9)

The published rectangle, dispatch/clock, audio-reuse, installer/converter,
blitter-completion, dirty-band, heap-search and physical-cache-observer helpers
are integrated into AmigaXDev main. Consumers still need their own behavioral
and workload gates before adopting runtime changes. The cache observer remains
an opt-in **68040 physical data-cache overlay**, separate from logical MMU
translation. Shared setup now installs `elf2hunk-fixed` separately; rebuilding
it reproduces this port's pinned converter hash and preserves the bundled tool.

Both ports import the canonical `amigaxdev.aprof` parser and call-tree traversal,
with per-port SHA-256 verification before capture use. Symbols, bucket names
and workload denominators remain local. Shared controls cover accounting,
overflow, missing/disconnected parents and aligned stop tails from zero through
one colour clock, rejecting negative, misaligned and oversized tails. The
unassigned stop tail remains separate from every function cost. Relocating an
identical reader passes; changing its source fails before a capture is used.
The existing scoped-capture gate still rejects seven incomplete captures and
two accounting faults.

MEASURED: the debugger's cached `regs.sr` can disagree with the CPU's separately
maintained condition flags. The canonical register-observer patch materializes
SR for both all-register and single-register replies. Two authored repeats on
each of 68030 and 68040 reproduce stale baseline values and then match all ten
CPU-saved CCR cases with the correction. All five flags are exercised, both
packet forms agree, the CPU's own assertions pass, and every fixture returns
normally. Twelve corrupted-evidence controls fail. This validates the debugger
read correction; it does not reinterpret earlier raw `$sr` samples as valid.
GDB's host register cache still needs flushing after monitor-driven execution.

The cinematic reporter remains flat-only. It now reports the shared pairing
rejection explicitly; unmatched returns cannot establish inclusive or active
costs. Source inspection and the bounded opening diagnostic identify deliberate
Line-A-to-user-mode bridges among these returns. That observation is not proof
that every return is paired. The legacy opening `work` output remains explicitly
approximate, distinct from D13's separately captured active scopes. The shared reader reproduces the matched `carpet-e/f` and `throne-clean-c/d`
flat reports; the earlier reports are retained unchanged. No new
cinematic inclusive or active-work result is claimed. Establishing workload-specific
return pairing remains a prerequisite for any future such result, rather than
an assumption made by the shared parser.

MEASURED: the correctness-only C2P breakpoint is installed at gameplay entry,
not conditionally evaluated throughout startup. `.run-b8X0WU` retains all 81
publications and passes full-plane equality and minimum dirty coverage. Phase
publication counts remain **7, 28, 9, 9, 28**, while stored row counts remain
**6, 28, 9, 9, 29**. Existing comparisons retain their row denominator; a cost
per actual publication must use the publication counts explicitly.

Alone in the Dark uses the canonical planner bodies through its existing
namespace adapter. Its clean ordinary executable is byte-identical before and
after sharing. The 400-frame host oracle and 4,096 minimum-row cases pass; the
native fight gate checks 40 publications (38 multi-rectangle), with 187 verified
frames and zero pixel failures. Its ordinary opening gate passes all 106 logo
and 840 book steps. These are correctness checks, not transferred performance
claims. The other shared runtime optimizations have not been silently adopted
by that port.

### Matched opening reports

`item19-final-before-1/2` and `item19-final-after-1/2` use the same profiling ELF
(SHA-256 `d188d671d157e9c23617546b6e10e88fe925c4c79f84578d153300d7b717c16b`)
and the unchanged pinned exact no-MMU 68030 profiler. Each has `opening`,
`report` and `work` output for phases 0–4. All four meet **81/81** original
SetTimer intervals from `opening-noright-1`; repeat comparisons retain all 81
Kid records and phase/stall boundaries. Both pairs span 375–376 PAL fields.
This sharing change makes no game-speed improvement claim.

Values below are ms per stored row; paired values are the two repetitions.
`work` is the legacy approximate tree estimate, not a new scoped D13 result.

| Phase | Rows / publications | Profile before | Profile after | `work` before | `work` after |
|---|---:|---:|---:|---:|---:|
| Landing | 6 / 7 | 94.01 / 93.89 | 94.01 / 93.31 | 76.95 / 77.65 | 76.66 / 77.27 |
| Health 3 | 28 / 28 | 82.81 / 82.73 | 82.84 / 82.73 | 39.33 / 39.21 | 39.35 / 39.37 |
| Health 2 | 9 / 9 | 98.30 / 97.81 | 98.11 / 97.82 | 38.02 / 37.96 | 37.84 / 37.74 |
| Health 1 | 9 / 9 | 98.84 / 99.56 | 98.95 / 99.55 | 38.96 / 39.58 | 38.67 / 39.39 |
| Death | 29 / 28 | 98.56 / 98.49 | 98.56 / 98.50 | 24.10 / 24.53 | 23.95 / 24.48 |

### Common regression evidence

The fresh `integration-gate-iajogfz3` passes the original title/first-room
comparisons, repeated route output and restoration. Its no-Right control starts
in the same room and reaches a different completed outcome. It rejects 20
altered title-native captures, six title-reference alterations and 19 altered
route captures. The four original service contracts also pass with ten rejected
alterations. The host suite, scoped-profile controls, cinematic controls and
shared-reader source-pin controls pass.

The fresh 68030 VBL fixture (`.run-BdAxI1`) passes the original countdown/removal
contract, user-mode ABI and six-field deferred dispatch, with 13 rejected
alterations. The Time Manager fixture (`.run-6zuiDb`) passes original delay
units, active prime, cancellation, zero delay and rearm, ABI and restoration,
with 12 rejected alterations. These service fixtures are separate from gameplay
cadence evidence.

The fresh cadence run (`.run-Ur7Fe5`) passes **59 consecutive original five-tick
intervals**, with six rejected alterations. It records exactly 59 conversions
for 59 updates, all at frame end. The ordinary `make release` rebuild passes
the link/release audits with no runtime hashing, trap trace or hash counters.
Its executable remains byte-identical: SHA-256
`957389f5595be880ae57e7c5ab45755140705809672ac28a726651f58731b179`.

`manifest/profile-sharing-evidence.sha256` pins the retained captures and reports;
`manifest/profile-helpers.sha256` pins the installed shared reader, and
`manifest/references.tsv` identifies the canonical shared and Alone in the Dark
commits. Captures remain ignored. These emulator checks make no hardware claim.
