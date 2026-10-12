# Open work

In playbook order. Each item's acceptance gate passes before the next item's work is
committed, except where an item says it runs in parallel.

1. **Owner decision D9** on "Compress Data" (`docs/design.md`).
   Acceptance: the decision table records the owner's answer.
4. **Static and live map.** Headless Ghidra with shared scripts; MacsBug names seeded
   into `disasm/symbols.csv` with evidence classes. A flow-following trap census
   (Alone in the Dark's `trap_census.py` without CREL), low-memory scan, A5-world
   model for `DATA`/`ZERO`/`DREL`, and a classification of every code write (CODE 1
   runtime, `MDRV` decryption). Locate the frame-publication, timer, `GetKeys`,
   `Jnth`/`MDRV`, file, copy-protection and packing seams. Check for a reduced-size
   renderer (`DrawHalf`, `ComputeHalfRect`) to close D4.
   Acceptance: the live trap log is a subset of the static map; byte classification
   coverage is reported; D4 is confirmed or reopened with the owner.
5. **Scaffold.** Copy Alone in the Dark's runtime (`src/mac/`), Amiga platform layer,
   framework with `UPSTREAM.md`, Kalms C2P, `amiga/` scripts, checker discipline,
   WHDLoad slave and release tools, without its game-specific hooks; pin the source
   commit in `manifest/references.tsv`.
   Acceptance: host tests and an empty Amiga executable that `amiga/run.sh` boots
   and quits cleanly, proven by a probe.
6. **Core runtime.** Loader and A5 world with byte checks, original CODE 1 unchanged,
   low-memory shadows, 3 MB application zone, reference identity answers, loud stops.
   Acceptance: a headless run reaches the first unimplemented service as a named stop
   after original initialisation, matched against the reference trap order.
7. **Files and resources.** Mohawk resource files, `MHWK` data-fork archives, the
   file calls the reference path uses, saves, `Prince2.opt` and Hall of Fame.
   Acceptance: paired read/write traces against the reference; saves survive restart.
8. **Display (D3).** 640×480×8 logical screen and GDevice, GWorlds, CopyBits and
   palettes from Alone in the Dark; a new PAL hires interlaced 8-plane FMODE 3 screen
   showing WIND 400 1:1, with Vette's interlace parity handling; dirty-rectangle C2P;
   palette fades applied at publication.
   Acceptance: title and first room match MAME captures at colour-index level, and
   the AGA planes and copper decode to the same picture.
9. **Input and UI (D6, D8).** KeyMap and event queue from the VBI, Mac UI removed,
   port-owned replacements for reached menu functions, copy-protection bypass.
   Acceptance: every reached UI path is listed with its replacement and covered by
   a probe; no Mac window chrome, menu or pointer is drawn.
10. **Audio (D7)**, parallel with 8–9. Alone in the Dark's native driver behind a
    port `Jnth` 11, re-measured for MIDI Synth 3.45; songs from `MIDISnd.dat` and
    `NISMIDI.dat`; samples from `DigiSnd.dat` and `NISDIGI.dat`; Paula DMA voices
    only, with measured voice priority.
    Acceptance: paired audio event logs against the reference for title, gameplay,
    death and one cutscene.
11. **Timing.** 60 Hz ticks from PAL fields, Time Manager and VBL tasks at safe
    user-mode points, completed frames paced at vertical blank.
    Acceptance: game-step cadence matches the reference on the fixed 68020 and on
    fast CPU configurations.
12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Acceptance: coverage matrix with positive completion records for every entry.
13. **Performance.** Profile the end-to-end build on `a1200-020` and the 68030
    reference configuration before optimising; set the Fast RAM requirement (D5).
    Acceptance: matched-workload measurements for each change, A/B against parent.
14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, 68020 and AGA checks; 68040 MMU
    test; deterministic LHA.
    Acceptance: the playbook §14 release checklist, including real hardware.
