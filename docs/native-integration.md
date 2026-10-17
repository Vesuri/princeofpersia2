# Native startup integration

The verified native route executes original startup, the opening cutscene, title
and first playable room on the Amiga. The original game consumes Space to start
and Right during gameplay. Full first-room pixels, colours and curated state
match the natural Macintosh reference. The title rendering comparison uses a
separate state-paired original fixture, described below. Timing and accepted all-level coverage are described in their dedicated reports.

## Reproduction

`make integration-contracts-check` creates four fresh Macintosh references and
checks the fixed-point, font, GWorld and title-copy contracts below. Each completed scratch
disk is compressed and verified by its decompressed SHA-256 before its redundant
uncompressed copy is removed. Original inputs are preserved.

For existing service-reference directories, source `amiga/env.sh` and run:

```
python3 tools/check_integration_contracts.py --references FIXED FONT WORLD COPY
```

The arguments identify, in order, references captured with
`POP2_INTEGRATION=1 POP2_FIXED_PROBE=1`, `POP2_INTEGRATION=1`,
`POP2_NEWGWORLD=1 POP2_DISPLAY_ROUTE=1`, and
`POP2_TITLE_COPY=1 POP2_TITLE_COPY_ALL=1 POP2_DISPLAY_ROUTE=1`. Reference files remain ignored.
The checker requires the pinned evidence and rejects ten deliberately altered
or incomplete inputs, then checks the restored positive evidence again.

`make integration-check` includes those contracts, portable helper checks and
audio-input verification, then runs the full native integration gate with
`VERIFY=1`. Ordinary builds disable runtime hashing, trap/resource/volume logs
and verification snapshots. The native
gate compares two original references and two natural native routes, runs a
separate state-paired native title fixture, exercises a no-Right
input control, and rejects deliberately altered completion, state, display and
restoration evidence. Its acceptance requires:

- Ordered title-start, title, first-room and completed original death milestones.
- Exact 512×384 title indexed pixels and colours against the state-paired
  original renderer fixture; exact first-room pixels and colours against the
  natural Macintosh route.
- First-room AGA planes, copper palette and display mode matching the reference.
- Matching first-room RNG and the curated context words.
- Original Event Manager consumption of Space to start and GetKeys reads of Right; a completed no-Right route with the same initial
  room and a different outcome.
- Original-code and input audits, clean heap and machine restoration, and Shell
  completion.

The integration build uses test-only wrappers for fixed seed, milestone capture
and timed input. Original game routines still execute. The default route lets the
opening cutscene finish naturally; it does not press Escape to bypass the title.
A deadline, missing milestone or named unsupported-service stop fails the gate.
The full integration gate passes, including two positive native routes, 20 title-fixture and 19 natural-route
altered-evidence rejections and the no-Right control. Six altered title
reference fixtures are also rejected. Audio and UI regressions pass, as do a
fresh native save replay, changed settings across reboot, and rejection of writes
to immutable installed data. These regressions retain their separate service
coverage; they do not extend the gameplay route to other rooms or levels.

A native diagnostic now consumes Space at the naturally reached title logo,
then reaches the first room and completes the original death routine after
reading Right. Its indexed pixels, colour entries, AGA planes, copper palette,
RNG and curated context words match the pinned Macintosh reference. Actual
Space consumption distinguishes this run from the automatic demo. The full
checker compares the title against a state-paired original rendering fixture.
A no-Right diagnostic starts from the identical room with zero Right reads and
completes a different death outcome. The full repeated gate verifies these results.

MEASURED original demo exit calls `HPurge` (CODE 3+5f9a) on a disposed master slot
and receives `memWZErr` (−111) in D0 and MemError. The native diagnostic now matches
both results and preserves the free-slot contents. Time Manager callbacks must
preserve MemError rather than overwrite it with the heap's last result. Unknown
handle addresses remain loud stops. `POP2_DEMO_ROUTE=1` with `POP2_HANDLE_TRACE=1`
selects the separate original diagnostic; its completion marker and seed checks
are distinct from the normal game route. Its log hash is pinned in
`manifest/integration-demo-reference.sha256`.

The title callback argument is an advancing animation-frame counter. Natural
native input follows the first completed logo frame, identified by cue 114 after
CODE 15+70ca/70cc selects it. Its frame number depends on service cost (frame 70
in the item-19.1 030 run); comparing that image to a fixed frame-16 reference
would compare different cloud positions.

`POP2_TITLE_STATE=16 POP2_DISPLAY_ROUTE=1 POP2_NATURAL_OPENING=1` selects a separate
original-renderer fixture. At guarded CODE 15+6fec, frame 15, it verifies cue 100
and supplies cue 101. Original code then selects and draws the logo; frame 16
must report cue 114 before capture and explicit completion. Two independent
original fixture runs match `manifest/integration-title-state-reference.sha256`.

The separate native `POP2_INTEGRATION_TITLE_STATE` verification build supplies
that same guarded input at frame 15. Its complete viewport and colours must
match the original fixture at frame 16/cue 114. It also completes gameplay and
restores hardware. This controlled renderer check does not prove natural title
timing. The two natural native routes and no-Right control are built without
that define; their receipts are checked, and their first-room state/pixels are
compared to the natural Macintosh references. The altered-evidence controls
cover the title fixture and natural routes separately. Release builds reject
the title-state define.

`make integration-check` creates the natural and state-paired references. To use
existing verified captures, run `tools/check_integration.py` with
`--references NATURAL NATURAL_REPEAT --title-reference TITLE_STATE` after sourcing
`amiga/env.sh`. Add `--setup a1200-030` to validate that selected machine; it
applies to the launcher and every result/rejection check. The Make target accepts
these options through `INTEGRATION_CHECK_ARGS`; `INTEGRATION_CONTRACT_ARGS`
likewise passes the four existing service references to the contract checker.
`--completed-native TITLE NATURAL REPEAT` can recheck completed native runs:
the gate rebuilds each variant and requires matching executable hashes and flags,
verifies the retained executable/symbol hashes, and reruns every result/control
check. The no-Right control still executes afresh. Reference hashes and completion
checks are identical in fresh and reuse modes. Missing Space consumption, mismatched title frame/cue, any viewport
pixel mismatch, or missing route completion fails the gate.

The fresh integration references leave the opening uninterrupted. The reference
A5 hash uses a virtual `first-room-a5.normalized.bin`: the raw snapshot with only
its four-byte cursor-idle timestamp (A5−0x53aa) and Time Manager countdown
(A5−0x7aa) zeroed. DERIVED from CODE 2+3a60/3a64 and CODE 21+6a62/6a66/6a68;
MEASURED repeated original runs differ only in these timing fields. Raw captures
remain intact. Room context, RNG, pixels and colour entries are not normalized.

## Measured service contracts

- **Fixed-point arithmetic:** 80 original `FixMul`/`FixDiv` results cover signs,
  fractions, saturation and zero divisors. Exact half-unit division underflow
  returns zero; nonzero half ties round up. The portable implementation is checked
  under address and undefined-behaviour sanitizers.
- **Fonts:** original calls verify two font-metric results, two character widths
  and one string width. Both reached NFNT resources reject every truncation in the
  portable parser check. This does not by itself prove every rendered glyph or
  opening palette entry.
- **GWorlds:** 23 original allocations verify normalized public CGrafPort and
  PixMap records. Reached sizes are 512×384, 510×87 and 510×384, all with 528-byte
  rows and the measured noNewDevice form. The last size is the title buffer.

- **Title copies:** all 65 offscreen copies match complete destination buffers,
  including preserved row padding. One copy writes to an unlocked GWorld while
  the visible window remains current; 64 copies read that unlocked GWorld into
  the current offscreen port. Source pixels and PixMap lock states remain
  unchanged. Port clipping applies only for a current destination, consistent
  with the [Inside Macintosh CopyBits contract](https://dev.os9.ca/techpubs/mac/QuickDraw/QuickDraw-166.html).

The corresponding `manifest/integration-*-reference.sha256` files pin the
reference contracts. No original font, record or captured frame is tracked.

## Active Time Manager tasks

MEASURED on the original System 7.5.5 route: the MIDI callback primes its task
inside CODE 22+6b90, then primes it again at CODE 22+7028 while active. Both calls
return success. The first requests of 172, 28 and 123 ms are followed by callbacks
10, 2 and 8 ticks later, despite second requests of 416, 272 and 367 ms. The active
prime preserves the original deadline; it does not reschedule it. The native
service now follows this observed behavior and passes the full native integration
gate. This establishes the reached startup contract, not item 11 timing fidelity.

`POP2_TIMER_TRACE=1` adds byte-guarded before/after probes at both calls. The
completed reference contains 1,939 callback pairs and matches the pinned room
state and pixels. Its diagnostic log identity is in
`manifest/integration-timer-reference.sha256`; the log itself remains ignored.

## Reuse boundary

`RectBounds.h`, `BitmapFont.h`, `GWorld8.h`, `CopyBits8.h`, `FillRect8.h`,
`WindowGeometry.h`, `RegionRows.h`, `RegionOps.h` and `BigEndianMemory.h` are
canonical AmigaXDev helpers, vendored identically in this port and Alone in the
Dark. `BitmapText8.h` is also canonical, adapting Alone in the Dark's clipping
and pixel addressing for original NFNT glyphs; only this port currently uses it. Their hashes are checked
before builds. NativeDisplay enables the shared region-union extension with
`AMIGAXDEV_REGION_UNION`; Alone in the Dark retains its existing operation set.
Reuse revisions are pinned in `manifest/references.tsv`.

`PcmDma.inc` shares Alone in the Dark's phase-preserving sample conversion
through AmigaXDev. The donor port's executable remained byte-identical when it
adopted the shared file; portable checks cover alignment, decimation and loop
boundaries. Prince of Persia 2 uses that same converter through its DMA plan
adapter. This does not establish full-route timing equivalence.

`CodeRangeCache.h` is also canonical AmigaXDev code, currently used by this port.
It revalidates each cached CODE range against the current segment table and
falls back to the complete search on misses. Boundary, replacement, truncation
and collision checks compare it with the uncached lookup.

`ResourceDirectory.h` and `ResourceDirectory.cpp` are also canonical sources,
shared with Alone in the Dark. This port enables the sorted lookup adapted from
Vette, preserving the oldest surviving duplicate identity and rebuilding after
metadata mutations. Both lookup configurations pass an independent mutation
oracle; the indexed configuration passes the existing directory round-trip checks.
Alone in the Dark retains its linear configuration and byte-identical executable.

New trap adapters also use existing MacHeap, region, fill and GWorld helpers.
GetMouse and Button follow the other ports' Event Manager state conventions,
with the pointer removed under D8. The measured GWorld row layout differs from
Alone in the Dark's keepLocal form, so its game-specific stride is not copied.

Timing dispatch and several trap adapters still have local implementations.
In particular, Alone in the Dark preserves queued VBL ticks and queue passes;
the current local scheduler can collapse elapsed ticks. Full timing equivalence
is unproven. These services must not be described as fully shared or as having
passed item 11.

## Release startup check

The release-only observer stops at the original CODE 15 + $0416 first-text call
(text $6978, page 1), without injecting input or changing original state. It
compares the complete 512×384 viewport against the pinned Macintosh opening
capture. The executable symbol audit rejects runtime hashing, trap traces and
integration input probes. Missing completion and altered-pixel controls must fail.
This opening checkpoint passes on both a1200-fast and a1200-040; it does not
establish a startup speedup or replace the first-room integration gate.

With a completed original core reference directory, reproduce using:

```sh
. amiga/env.sh
make release
AMIGA_SETUP=a1200-040 CORE=1 RELEASE_STARTUP=1 \
  RELEASE_REFERENCE=/path/to/completed/reference \
  GDBSCRIPT="$PWD/amiga/release_startup.gdb" DEADLINE=240 /bin/bash amiga/run.sh
```

## Diagnostic continuation flags (item 21)

DERIVED: ten opt-in assembly observers used `MOVE.L` to push their continuation
address after restoring CCR. That overwrote N/Z/V/C. The wrappers now reserve
that slot with flag-neutral `LEA`, populate it before restoring CCR, then restore
registers and execute `RTS`. Title/death entry callbacks still receive the
original stack pointer. This follows Alone in the Dark's user-mode VBL
trampoline and this port's scene observer; inspection of Vette's geometry
observer found no additional reusable service body to move into AmigaXDev.

MEASURED: the actual assembled wrappers, linked with authored callbacks that
clobber every register and CCR, pass 640 paired continuations on the reference
Macintosh CPU and native `a1200-030`: ten wrappers, two register patterns and all
32 CCR combinations. Every D0–D7/A0–A6 value, stack balance, caller-stack canary
and callback argument agrees. Observation occurs after the real wrapper `RTS`,
before a flag-changing capture instruction. The Macintosh harness restores its
original CPU state before exiting. This is an authored ABI fixture executed on
the original machine, not a claim that gameplay naturally exercises every CCR.

The pre-fix assembly is a negative control: 600 of 640 cases corrupt CCR, across
all ten wrappers, while the other values agree. The checker also rejects 33
altered results, missing completion/restoration signals, memory-access errors,
wrong CPU configuration, unpinned inputs and modified executable evidence.
The first Macintosh trial had invalid teardown and remains rejected; the
accepted repeat includes verified restoration and no fixture-scoped unmapped
access. Input and evidence identities are in
`manifest/wrapper-ccr-inputs.sha256` and `manifest/wrapper-ccr-evidence.sha256`.

The accepted pair is `ref/mame/runs/wrapper-ccr-fixed-2` and
`tmp/wrapper-native-rnsmnjp2`; `tmp/wrapper-native-w0s91r5u` is the pre-fix
negative control. Rebuild the authored image with
`python3 tools/build_wrapper_fixture.py tmp/wrapper-ccr/fixed` after sourcing
`amiga/env.sh`. Original execution uses `POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1
POP2_WRAPPER_FIXTURE=tmp/wrapper-ccr/fixed` with `tools/run_reference.py`.
Native execution uses `tools/run_wrapper_fixture.py tmp/wrapper-ccr/fixed`.
Recheck the recorded pair with `python3 tools/check_wrapper_ccr.py --original
ref/mame/runs/wrapper-ccr-fixed-2 --native tmp/wrapper-native-rnsmnjp2 --controls`.
`tools/wrapper_live.py generate` adds one-shot observations immediately after
the wrappers save CCR, guarded by their ten-byte instruction prologue, to the
existing gameplay scripts. `POP2_WRAPPER_LIVE=1` adds save/load return samples
at byte-checked original `RTS` sites; `wrapper_live.py check` compares D0 and CCR.
All fixture images, raw rows, captures and emulator state remain ignored.

MEASURED: `tmp/integration-gate-cvim5b2n` passes the original-paired service
contracts, title-renderer check, two natural startup/death routes and the
no-Right control on `a1200-030`. The two natural runs reproduce first-room and
death pixels; the control starts from identical pixels, consumes zero Right
reads and completes with a different outcome. The gate also rejects 20 altered
native title observations, six altered reference observations and 19 altered
natural-route observations. Restoring `make release` reproduces executable
SHA-256 `c913cb3947c50d1cfe99297bec4636953ce9a7b93b7236101cdd62f8c6641247`.

MEASURED: the restart pair in `tmp/gameplay-restart-gate-4huqb_tg` and save/load
pair in `tmp/gameplay-save-gate-rbxb6o28` pass their original comparisons and
15/22 rejection controls. The first restart capture was initially checked against
an older original log lacking the menu-command sample; it passes unchanged
against `gameplay-restart-2`, with `gameplay-restart-3` providing the repeat.

Live save/load returns in `ref/mame/runs/wrapper-ccr-save-original-1` and
`amiga/.run-l3WUkZ` both have D0=1 and CCR=0. The native observation reads the
word written by `MOVE CCR`, before the callback runs; the ABI fixture verifies
that it survives the subsequent wrapper return. Earlier raw GDB `$sr` samples
in `amiga/.run-Nkb2ni` and `amiga/.run-eW5nEu` reported CCR=4 at entry and are
rejected as flag evidence. The CPU-saved observation and complete save/load
route pass. Investigating the debugger's stale SR report remains in shared-tool
item 19.9; no game behavior was changed to accommodate that report.
Four live-observation controls reject altered D0, CCR=4, a missing sample and
missing route completion.

MEASURED: ending/Hall run `amiga/.run-Vlasd1` passes 81 original-paired
checkpoints, 385 frames, 387 consumed key reads, natural ending completion and
the persisted Hall name/score. The one-shot observations plus the normal
integration death-entry/return markers cover all ten changed wrappers.
`tmp/wrapper-ccr/report.json` records the complete gate and unchanged release
identities. `make release` is restored; item 21 is complete.
