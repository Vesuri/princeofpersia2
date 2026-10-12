# Static and live map

The map uses original CODE resource offsets, including the four-byte segment
header. `MEASURED` means original bytes or original execution; `DERIVED` means
control-flow or host-model analysis. All images, listings, traces and generated
maps remain in ignored `tmp/` and `ref/`. The [core runtime](core-runtime.md) now validates the bounded original startup on Amiga.

## Reproduction and scope

`make map` verifies the extracted inputs, follows control flow, and writes
`tmp/static-map/map.json`, `traps.csv`, `lowmem.csv`, `byte-ranges.csv`,
`unresolved.csv`, entry points and Ghidra symbol input. The census adapts Alone in
the Dark's walker without CREL. `make ghidra` imports the 20 nonempty CODE segments
with the pinned shared `MacMarkEntries`, `MacDumpTraps`, `MacApplyNames`,
`ExportAuditListing` and `DumpZeroPage` scripts. The three four-byte editor stubs
are classified as headers. Ghidra exports have an input-hash receipt; script
errors or changed inputs fail the run. Its generic zero-page script is only a
supplement: the Python scan covers absolute Macintosh addresses below $2000 and
excludes LEA/PEA address constants from memory-access counts.

Set `GHIDRA_HOME` to override the shared Ghidra 12.1 installation under `~/.local`.
`JAVA_HOME` is honored; otherwise the runner locates Homebrew's OpenJDK 21.
`AMIGA_GHIDRA_SCRIPTS` comes through `amiga/env.sh`. Engine, language and script
hashes are pinned in `manifest/ghidra-tool.sha256` and `ghidra-scripts.sha256`.
The Capstone 5.0.7 Python binding and native decoder are verified against
`manifest/capstone.sha256` before import.
The locally reused trap-name table is pinned in `analysis-inputs.sha256`; it is
ignored, with provenance in its generated header. On a fresh setup, generate it
with `python3 -m amigaxdev.mac.gen_trap_names` in the shared environment; the
pinned hash must match before analysis. Unknown names stay unknown.

`make static-map-check` runs Ghidra, then two fresh original Macintosh reference
runs with the optional map observer. It requires the existing frame and ordered
trap hashes, identical captured images, exact model comparisons, positive probe
completion, rejection controls and ignored-output checks. To recheck two retained
runs after `make ghidra`, pass their paths to
`python3 tools/check_map_gate.py --runs RUN1 RUN2` in the shared environment.

This is the opening-text checkpoint from the reference loop, not gameplay-wide
execution coverage. Unreached library paths and unknown indirect destinations
remain named unresolved transfers. Later runtime work must stop on them until
measured; this map does not authorize guessed service results.

## Byte classification and service surface

The flow map starts from all 1,018 jump-table entries, explicit runtime entries,
verified post-name prologues and referenced callbacks. It recognizes bounded
PC-indexed switches, the StripAddress trampoline, and the return-address getter
that exposes CODE 19's saved-A5 slot. It does not linearly interpret name strings
or arbitrary resource words as instructions. There are 1,606 unique roots.

| CODE byte class | Bytes |
|---|---:|
| Followed instructions, including A-line words | 332,166 |
| Switch data | 576 |
| MacsBug name records and their alignment | 16,278 |
| Segment headers | 92 |
| Runtime data inside CODE 1 and CODE 19 | 16 |
| Explicitly unresolved residue | 40,698 |
| Total CODE 1–23 | 389,826 |

DERIVED: instruction coverage is 85.2%; 89.6% of bytes have a more specific class
than unresolved. Every residue run is emitted with its exact extent. The bounded
74.3% CPU survey remains independently reproducible by `make inspect`; its more
limited seeds are not the complete map.

There are 1,487 direct A-line sites, 281 distinct trap words, 132 syntactically
resolved calls through five cached Toolbox addresses, and 438 remaining indirect
transfers. The direct low-memory scan finds 56 access sites at 28 addresses.
The map distinguishes access operands from constants passed as arguments; it
cannot prove the absence of additional register-indirect low-memory accesses.
The A5 references to cached services are derived from the setup in CODE 16.

MEASURED: all 338 distinct CODE trap sites in the 50,140-record ordered opening
trace are in the static census. The original expanded MDRV contributes 21 further
live trap sites, all in its own flow map. OS/ROM-internal calls are outside these
application/driver address spaces and are not mislabeled as application traps.
The checked live CODE sites are classified as instructions.

`disasm/symbols.csv` contains the 1,032 conservative RTS-adjacent MacsBug records,
plus curated runtime and service seams. Record labels end in `_MacsBug`: they mark
name data, not function entries. Entry labels are separately evidenced; names are
never assigned to an entry merely because a printable string is nearby.

## A5 world and runtime

MEASURED: CODE 0 describes 8,176 bytes above A5 and 22,266 below, with 1,018
unloaded jump entries at A5+32. There is no CREL resource. The near-model CODE 1
runtime differs from Alone in the Dark's far-model relocation path.

CODE 1+$00B4 expands DATA word-by-word; each zero word consumes a ZERO run length.
The streams consume exactly, with 1,286 runs. DREL contains 303 signed word
locations below A5; all tags are even, so all add A5, with no STRS relocations.
The checked host model matches all 22,266 bytes captured at CODE 1+$002A, before
the trap-stub installation and original main initialization. The unbased image
hash is `204dd49e7f7fa45c8b3cc453ce6ee68b3b50a15f1d751bbcfda82360121b9d0f`.

CODE 1+$0024 stores the STRS pointer at CODE 1+$0004 (data). At +$03AE it allocates
24 bytes and emits two JSR/JMP stubs for LoadSeg and ExitToShell. Their targets
and every emitted byte are checked. The LoadSeg hook at +$0064 clears the segment
header's relocation flag; the observed near headers already have it clear. It
then returns through the stub to the original system loader. The dormant CREL
path remains visible in the map, not an implemented port service.

CODE 19+$0484 stores A5 at +$0478, a four-byte data slot reached through a
return-address getter at +$0476. The getter removes its inner BSR return address;
those four bytes are never an instruction fall-through. Eight stores were
observed and the final data value is checked. All other final CODE bytes match
the verified resources, apart from the CODE 1 STRS pointer.

## Executable-region writes

The observer derives virtual-to-physical RAM translation from the original
CODE 1 STRS store and verifies each watched range against virtual reads. This is
necessary on the IIci's 24-bit-addressing configuration even with VM off. The
measured RAM offset is $50000; it is discovered, not assumed. MAME watchpoint
widths in this build are logged in bits. The observer tracks moved handles and
captures the final locked sound-driver address after MoveHHi/HLock.

| Observed write class | Write events | Check |
|---|---:|---|
| CODE 1 STRS data pointer | 1 | Physical calibration plus final pointer/image |
| Runtime stub construction | 10 bus writes | All 24 bytes and original handler targets |
| Near CODE header BCLR | 11 | Original header byte unchanged |
| CODE 19 saved A5 | 8 | Only the four-byte data slot, exact A5 value |
| MDRV decryption | 13,373 | Every byte from offset 7 through end, exact cipher model |
| MDRV LZSS expansion | 30,960 | Every output byte once, exact independent decoder |
| Installed MDRV mutable data | 848,806 | All writes at offsets $4600–$78EE; executable prefix unchanged |
| OS heap reuse during CODE 15 relocation | 710 | Before relocated segment publication; final complete CODE image checked |

These counts describe this bounded route. Every recorded event must fit a named
class; an injected opcode write is rejected. Stub construction and the complete
per-byte decryption/expansion provide positive controls, rather than accepting a
silent watchpoint. Moving a resource can retire its previous storage; writes to
that storage are not called self-modification of the new resident image.

The expanded MDRV is 30,960 bytes; its SHA-256 is
`cbd03e0121a8b2a73063b653c78b58059a48e02e51b7dc927b3bee15d0b8144f`.
Its selector dispatcher has 38 branch entries. The flow map classifies 17,346
bytes in its executable prefix, with 574 prefix bytes still unresolved and
13,040 bytes in its A4-relative mutable-data region. Four indirect transfer
records remain, including the explicitly seeded selector dispatcher. The raw,
decrypted, expanded and installed images are compared; the packed file is never
disassembled as though it were the executing driver.

## Port seams

| Seam | Original location | Evidence and boundary |
|---|---|---|
| Frame publication | CODE 17+$5020 and +$508A | CopyBits wrappers; five calls at +$5020 in the opening route. `PaintCopy` at +$55FA is an unscaled row copier, not the publication service |
| Timing | CODE 17 GetTimer +$3032, SetTimer +$3136, WaitTimer +$3184, WaitTimerNoPoll +$31B8 | TickCount paths; Time Manager sites are separately enumerated in CODE 21/22 |
| Keyboard | CODE 16+$2832/+283C; CODE 2+$2EFE, +$449E, +$5620 | GetKeys address stored at A5−8040 and invoked indirectly. Startup +$449E is positively observed with target equal to the installed slot |
| Audio | CODE 11 GetDriverCodeResource +$2212, DecryptHandle +$21B8, LZSS decoder +$3858 | Jnth lookup precedes MDRV fallback; original driver initialization calls through the stored entry at +$32F4. The audio build supplies the D7 native Jnth driver; see audio.md |
| Files | CODE 21 HighLevelFSDispatch sites, beginning +$0BCE; resource calls across CODE 11/17/21/22 | Census retains call-site contexts/selectors; fork-based and Mohawk data files remain distinct. No file-service implementation is claimed |
| Copy protection | CODE 2+$1A36; choice at +$02F8 | Original cutscene selector 7 avoids challenge selector 100 in this route; no bypass implemented |
| Packing | CODE 2+$3CE8; CODE 17+$4560 and PackAllData +$41A6 | Original options 0/1 skip the prompt. D9 is still awaiting the owner; no packing policy change |

## D4: DrawHalf is not a reduced-resolution renderer

DERIVED: DrawHalf at CODE 3+$3480 calls ComputeHalfRect at +$35BC, saves and
substitutes the active scene rectangle at A5−17920, invokes the existing drawing
path and marks the resulting rectangle for redraw. ComputeHalfRect selects
shape-dependent scene rectangles (including the 135–144 case range); it does not
halve destination coordinates or pixel strides. DrawHalf's painter callback and
the normal drawing path share the original-scale buffers. PaintCopy preserves
width, row count and source/destination strides.

The two named routines therefore provide no faithful lores display alternative.
The owner confirmed dropping lores after this finding. D3 remains the 512×384
window at 1:1 on PAL hires interlaced. The [display gate](display.md) validates
its separate native presentation implementation.

The completed gate reports `STATIC MAP GATE PASS`: two original runs, identical
captured image hashes, twelve rejecting corruption controls, and no tracked derived
output. Four focused flow/model fixtures and the MAME literal/syntax audit pass.
