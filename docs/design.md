# Port design

## Owner decisions

| ID | Decision | Status |
|---|---|---|
| D1 | Port Macintosh Prince of Persia 2 to Amiga following Vette! and Alone in the Dark: original 68k instructions run natively, Macintosh services are implemented by the port | Decided |
| D2 | Reference system: System 7.5.5 under MAME on a compatible Macintosh IIci, 8 MB, main `mdc48` display at 256 colours, Virtual Memory off. Owner-authorized fallback from the failing System 6.0.8 / Macintosh IIx configuration | Decided |
| D3 | Display: PAL hires interlaced, 8 bitplanes, FMODE 3, showing the original 512×384 window 1:1. No DblPAL or other 31 kHz mode | Decided |
| D4 | Drop lores; retain D3’s 512×384 display at 1:1. `DrawHalf` and `ComputeHalfRect` select scene rectangles at the original scale, not a reduced-resolution renderer. Confirmed by the owner after analysis | Decided |
| D5 | Current target: AGA, 68030 or better, 8 MB Fast RAM. Current validation uses only `a1200-030`. Owner deferred 68040 validation to release work and subsequently requested 68020 item 15 ahead of release. The 68020 target is being evaluated, not yet accepted. Release startup fails at 4 MB and passes at 8 MB on the measured a1200-030 configuration | Decided |
| D6 | Bypass the copy protection at a measured, byte-verified seam | Decided |
| D7 | Music: reuse Alone in the Dark's native Halestorm driver in place of the original `MDRV`, improving it as needed. All audio through Paula DMA voices; no software mixing | Decided |
| D8 | Remove all Macintosh UI: menu bar, window frames, dialogs and pointer. Reached functions get port-owned replacements | Decided |
| D9 | Do not support "Compress Data". Installation supplies uncompressed data, which stays uncompressed; no in-place packing or startup packing prompt | Decided |

## Assisted gameplay validation

D10 (owner-authorized): item 12 may use cheats and checkpoints to bypass traps
and combat. Its purpose is to expose loud stops and establish that the port can
reach completion, not to demonstrate unassisted playing skill. Record every
intervention, apply equivalent assistance to original/native comparisons, and
retain positive endpoints for levels, cutscenes and session actions. A level
loaded by a cheat is coverage of loading and entry, not proof of its exit or
ending. Harness assistance is opt-in and excluded from ordinary release builds.

D11 (owner-authorized gameplay acceptance): the already recorded torch/RNG
state and pixel differences remain in deferred item 16. Item 12 may accept
completed assisted routes with those exceptions explicitly identified, provided
there are no unresolved loud stops and the game reaches the ending. This does
not turn unexecuted routes or unreached endpoints into passes.

D12 (owner-authorized item-12 closure): accept the recorded routes through all
levels and the ending while deferring the observed one- or two-frame exit
counter differences and remaining level-3/4/13 animation, RNG and actor-slot
differences to item 16. The gameplay relevance of differing non-player actor
slots is not yet established. Preserve the failed strict comparisons and their
captures; this acceptance does not claim exact equivalence or waive unknown
service calls, unreached exits, or new differences on other routes. Positive
original/native exit and session-action evidence remains required. Release/040 validation remains deferred; the subsequent owner request brings 68020 item 15 forward without changing these gameplay exceptions.

## Performance measurement scope

D13 (owner-authorized): apply item 19.2’s 12 ms/frame dispatch budget and
item 19.3’s 2 ms/frame clock budget to active game work, excluding the original
`CODE 17+$31B8 WaitTimerNoPoll` timer-wait loop. Obtain new scoped captures;
existing aggregate reports cannot be relabelled as active-work measurements.
Retain the 25 µs per-trap limit, full-scene cadence and all correctness gates.
The owner subsequently authorized doing audio-cost item 19.6 next, keeping
19.2 open and returning to its remaining dispatch and handle costs afterward.

## Faithfulness boundary

The original Macintosh execution under the D2 reference is the correctness reference.
Preserve game logic, data interpretation and decisions. The owner authorizes byte-verified low-memory address relocations while preserving
original instruction lengths and startup logic. Port-owned services handle
Amiga display, audio, input and platform integration. Any behavior-changing departure
requires an explicit owner decision and validation against the original.

The identity the port reports (`Gestalt`, `SysEnvirons`, Virtual Memory, MIDI Manager,
screen depth) selects the original's code paths, so it must equal the chosen reference.

Vette! and Alone in the Dark are the reuse references. Their inspected commits and
the shared playbook baseline are recorded in `manifest/references.tsv`.

## Display

The game draws into WIND 400, a 512×384 content rectangle at 8 bits per pixel. A PAL
interlaced hires frame shows it without scaling. With eight hires planes every Chip-bus
slot is taken at FMODE 0–2; FMODE 3 leaves roughly half (DERIVED, unmeasured). Presentation converts only
dirty rectangles, as in both reference ports. A 320 or 368 pixel lores width shows at
most 72% of a room, so the owner confirmed dropping lores after the original drawing-routine analysis (D4).
