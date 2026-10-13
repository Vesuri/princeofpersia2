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

## Measured baseline

The five-update 68030 window takes 25 PAL fields, or 100 ms per update including
pacing. This includes an initial longer step; the separate timer record shows
55 five-tick intervals, one six-tick interval and one ten-tick interval over the
recorded route. This does not establish all-scene cadence.

The instrumented C2P baseline has exactly five conversion passes, ten dirty
rectangles and 179,328 converted pixels over those five updates. No pass occurs
outside frame-end publication. A pass may call the Kalms rectangle routine more
than once, once for each rectangle; it does not redraw the full screen merely to
make one rectangle call. C2P and synchronization use the existing Kalms adapter
and the shared blitter path, following Alone in the Dark. Vette likewise converts
at its completed-frame boundary.

The required acceptance still includes the final comparison against the parent
commit and wider confirmation of frame-boundary behavior. Title phase pairing
and full timing correctness remain separate unfinished gates.

## Dispatcher candidate

The candidate makes known cached-service veneers bypass the original CODE-segment lookup. Their
return addresses still undergo the original owner check; trap behavior and
service implementations are unchanged. The first matched 68030 A/B window
changes from 25 to 24 PAL fields (100 to 96 ms/update). End screen/context bytes
and conversion work match exactly: five passes, ten rectangles, 179,328 pixels,
zero unbatched passes. Eight altered-evidence controls are rejected.

This is one short, field-quantized comparison. Repeatability and broader
correctness validation remain necessary before accepting a general performance
claim or completing item 13.

## Memory and correctness

The unmodified release build reaches the original opening checkpoint and matches
its complete viewport on the 8 MB Fast RAM preset. With the same CPU/cache
settings and 4 MB Fast RAM, it positively stops at `CORE / MEMORY`, without an
exception. Effective emulator settings confirm both memory sizes. D5 therefore
requires 8 MB Fast RAM; this is the supported configuration, not a claim that
every intermediate memory size was tested.

Profiling checkpoints leave 3,166,576 bytes of Fast RAM free at the opening/title
and 3,098,312 bytes at the first room/death; the latter largest contiguous block
is 3,096,912 bytes. Chip RAM has 1,147,864 bytes free. These are checkpoint
snapshots, not continuous high-water measurements or full-game coverage.

The current 68030 Time Manager and VBL fixtures match their pinned Macintosh
contracts, including user-mode ABI, CODE/heap audits and hardware restoration.
Both reject all twelve altered-evidence controls. Host helper tests and shared
source pins pass. Full gameplay coverage and fixed-68020 cadence remain in their
separate open items.
