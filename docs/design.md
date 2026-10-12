# Port design

## Owner decisions

| ID | Decision | Status |
|---|---|---|
| D1 | Port Macintosh Prince of Persia 2 to Amiga following Vette! and Alone in the Dark: original 68k instructions run natively, Macintosh services are implemented by the port | Decided |
| D2 | Reference system: System 7.5.5 under MAME on a compatible Macintosh IIci, 8 MB, main `mdc48` display at 256 colours, Virtual Memory off. Owner-authorized fallback from the failing System 6.0.8 / Macintosh IIx configuration | Decided |
| D3 | Display: PAL hires interlaced, 8 bitplanes, FMODE 3, showing the original 512×384 window 1:1. No DblPAL or other 31 kHz mode | Decided |
| D4 | Drop lores; retain D3’s 512×384 display at 1:1. `DrawHalf` and `ComputeHalfRect` select scene rectangles at the original scale, not a reduced-resolution renderer. Confirmed by the owner after analysis | Decided |
| D5 | Target: AGA, 68020 minimum, 68030 or better recommended; Fast RAM size set after measurement | Decided |
| D6 | Bypass the copy protection at a measured, byte-verified seam | Decided |
| D7 | Music: reuse Alone in the Dark's native Halestorm driver in place of the original `MDRV`, improving it as needed. All audio through Paula DMA voices; no software mixing | Decided |
| D8 | Remove all Macintosh UI: menu bar, window frames, dialogs and pointer. Reached functions get port-owned replacements | Decided |
| D9 | "Compress Data" (in-place LZ packing of the installed data files): not offered; the startup packing prompt is kept from appearing through the original's own check | Proposed, awaiting owner |

## Faithfulness boundary

The original Macintosh execution under the D2 reference is the correctness reference.
Preserve game logic, data interpretation and decisions. Port-owned services handle
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
