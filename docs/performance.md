# Performance

The current owner-directed target is `a1200-030`: the shared 50 MHz 68030,
AGA, PAL, 2 MB Chip RAM and 8 MB Fast RAM preset with data cache enabled.
68020 performance work is deferred. These are emulator measurements, not
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
window, or 104 ms/update including pacing. The final build takes 24 fields,
or 96 ms/update. Both perform five conversion passes, ten dirty rectangles and
179,328 converted pixels, with zero input in that window. Complete endpoint
screen and context bytes match. `manifest/performance-parent.sha256` pins the
parent executable, symbols and both benchmark and route endpoint captures.

The parent's full-route field sampler records 243 observations: 99 in original
game execution, 36 in TickCount, 32 in frame-end C2P, 22 in SystemTask and 17 in
SectRect. These are periodic phase samples, not instruction counts or exclusive
service costs. They identify dispatch/polling and conversion as relevant work.

The dispatcher now skips the original CODE-segment search for known cached native
service veneers; their original return-owner checks remain active. The isolated
change measured 24 fields against the parent's 26, with identical benchmark
screen, context and conversion work. The final frame-owner fix retains that
24-field result. These are short, field-quantized emulator windows; they do not
establish a general percentage speedup or all-scene frame rate.

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

The full tested route has 57 updates and 57 conversion passes, 109 rectangles
and 830,016 converted pixels, with no conversion outside frame end. The diagnostic
route before the owner correction had 58 passes for the same 57 updates,
rectangles, pixels and final screen/context bytes. Both final endpoints also
match the pinned parent. Converted bitplanes match the chunky image at the
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

Profiling checkpoints leave 3,166,232 bytes of Fast RAM free at the opening/title
and 3,097,968 bytes at the first room/death; the latter largest contiguous block
is 3,096,592 bytes. Chip RAM has 1,147,864 bytes free. These are checkpoint
snapshots, not continuous high-water measurements or full-game coverage.

The current 68030 Time Manager and VBL fixtures match their pinned Macintosh
contracts, including user-mode ABI, CODE/heap audits and hardware restoration.
Both reject all twelve altered-evidence controls. Host helper tests and shared
source pins pass. Full gameplay coverage and fixed-68020 cadence remain in their
separate open items.
