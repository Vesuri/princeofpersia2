# Display services and presentation

`make display` builds the original-instruction core with the display services
below. Original startup crosses `InitGraf` and stops at the named, unsupported
`InitFonts` call (CODE 2+$3E92). It is not yet a playable game. `make core` retains
the earlier bounded initialization gate.

`make display-check` reproduces the display gate: two original scene runs, a
paired service survey, a colour-transfer sweep, native probes and rejection
controls. It finishes with `DISPLAY GATE PASS`. Reports live in ignored
`tmp/display-gate-*/report.json`; reference captures and native run directories
also remain ignored. The gate leaves the `make display` configuration built.

## Macintosh evidence

MEASURED on the pinned D2 System 7.5.5 / IIci / mdc48 reference:

- The logical device is 640×480×8. WIND 400 supplies a 512×384 client. Its final
  client origin is (65,48); its PixMap bounds are translated accordingly.
- The reached `NewGWorld` form uses depth 8, `noNewDevice` (2), the main GDevice,
  no explicit colour table, and either 512×384 or 510×87 bounds. Both allocations
  have a 528-byte stride. They share the main device colour table. Locked maps
  use a raw pixel base/version 1; unlocked maps use a handle/version 2.
- The 256-entry palette uses `pmAnimated|pmExplicit` (12), with protected white
  and black endpoints using usage 2. `AnimatePalette` replaces RGB high bytes
  while retaining the existing low bytes. All 142 captured transitions match
  complete palette and logical colour-table records, including unchanged bytes.
- All five captured `CopyBits` calls use `srcCopy`, no mask, and the shared colour
  environment. Complete destination buffers match, including pixels outside the
  copied rectangles. Ordinary ADB input places the cursor outside the client for
  this survey; no cursor pixels are silently excluded from the comparison.
- A separate CPU-executed `SetEntries` fixture measures all 65,536 channel values
  and 256 mixed colours. The card's RGB output depends on the high byte only.
  This measured transfer is applied before writing AGA colours; copying the
  logical RGB high bytes directly would give the wrong displayed colours.

The original route uses Escape from the opening and Space at the title. It
captures the complete title logo at CODE 15+$6FEC, phase 32 / command 114, and the
first room at CODE 2+$6056 after `FirstFrame`. Both checkpoints are byte-guarded
against the verified original CODE resources. Two fresh runs give identical
640×480 indices, logical colour tables, hardware RGB palettes and PPM images.
`manifest/reference-display.sha256` pins these eight outputs.

`manifest/display-inputs.sha256` pins the measured transfer and the two-scene
fixture. The actual bytes stay under ignored `build/display/`; generated headers
contain hashes only. Native preflight verifies the files independently before
using them. No game pixels or measured colour-transfer table are compiled into
tracked source. A fresh `make display` measures the transfer if it is absent;
an existing mismatched table fails verification.

## Service boundary

`NativeDisplay` owns the main logical screen, GDevice and PixMap, public QuickDraw
globals, GWorld pixel handles, client-window ports and palettes. It implements
these measured forms of `InitGraf`, `GetMainDevice`, `NewGWorld`, pixel lock/unlock
and base lookup, GWorld selection/disposal, port selection, `GetNewCWindow`, window
position/visibility, RGB foreground/background, palette creation/binding/activation,
entry colour/usage, `AnimatePalette` and `CopyBits`.

Window records provide the display geometry needed by these calls. No window
chrome or pointer renderer is implemented. Other UI services still stop; their
replacements belong to item 9. Unmeasured depths, allocation flags, palette forms,
CopyBits masks/modes, unowned pointers and unknown QDExtensions selectors have
named stops. This is not a general Macintosh QuickDraw implementation. Private
ROM records and device inverse-table machinery are not emulated.

Native CPU probes compare public QuickDraw globals, complete GWorld port/PixMap
records with owned pointers normalized, main-device public fields and lock/unlock
pixel identity against the original. The scene probe then executes authored 68k
calls to the same dispatcher for window setup, palette realization/animation and
CopyBits. It loads the original captures into a locked offscreen GWorld, copies
them into the logical client and publishes them. Thus the scene gate tests
**display-service replay and presentation**, not execution of the original game
renderer on Amiga. Original native gameplay remains behind the UI, audio and
timing gates. The reference harness does not bypass copy protection for this route.

## AGA publication

The display owns two 512×384×8 Chip bitmaps and four immutable copper lists, one
per bitmap/field parity. Each row interleaves eight 64-byte plane rows. PAL hires
interlace uses 192 lines per field, FMODE 3, 8-byte-aligned pointers, and modulo
960: a 64-byte fetch plus 960 skips to the next row of the same parity.

DERIVED mode words: `BPLCON0=$8215`, `FMODE=3`, `DIWSTRT=$4CA1`,
`DIWSTOP=$0CA1`, `DIWHIGH=$2100`, `DDFSTRT=$48`, `DDFSTOP=$B8`.
The fetch window covers eight 64-pixel groups at hires. MEASURED in the pinned
FS-UAE A1200/68020/PAL AGA configuration: long fields select row 0, short fields
row 1; consecutive fields share the same bitmap and all 256 RGB24 colours.
Publication occurs on an even field. The observer checks the actual selected
copper list, both parity pointers and publication before line 16, with no late
updates. Emulator evidence is not a real-hardware release result.

CopyBits marks client rectangles dirty. The Kalms converter expands horizontal
spans to its 32-pixel width/16-pixel start constraints and converts only those
rectangles. Unchanged regions are carried forward between the two buffers.
Moving the viewport forces a full conversion. Palette-only publication preserves
all indices while replacing both field palettes. Fades use the measured transfer
and AGA high/low colour-bank writes at publication.

The independent host decoder checks all 196,608 pixels of each title/room client,
all eight planes, all 256 copper colours, field pointers, row modulos, mode words
and DMA bounds. A separate four-frame authored pattern checks initial conversion,
a partial update, palette-only change and viewport movement, comparing complete
frames after each step. Native input-hash and unknown-selector controls must stop;
missing scene/field completion, wrong parity, bad restore/Shell status, changed
pixels, palettes, pointers and mode words must fail the same successful checker.
Every positive native run also restores the OS View, vectors, DMA/interrupt masks,
and returns through the Shell with the expected diagnostic status.

There are no performance or final memory-requirement claims. Publication ownership
uses VBI; 60 Hz game scheduling and Time Manager/VBL task integration remain item 11.
The reusable source provenance is pinned in `manifest/display-sources.tsv` and
`manifest/references.tsv`.
