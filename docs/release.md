# Version 0.90 (16.10.2026)

Item 14 is in progress. No release archive has been qualified or published.
The owner will test an Amiga 1200 with 68040/40, 32 MB and MMU. Hardware
confirmation remains required; emulator observations do not replace it.

## Installer

`tools/install-data` adapts Alone in the Dark's plain C installer, using its
bounded StuffIt/Arsenic decoder through AmigaXDev's shared installer source.
This archive has no nested disk image or InstallerMaker payload. The native
helper verifies the whole archive, extracts all 96 fork/metadata files, and
regenerates four runtime inputs from the application: relocations, file catalogue,
UI seams and audio pitch. The measured video transfer input remains separate.
All 100 outputs match their existing manifest hashes in host tests. Repeated
installation, partial repair, preserving differing existing files, damaged
archive rejection, cancellation and staging cleanup pass. The native helper reproduces all 100 outputs with a 4096-byte stack
(3272 bytes unused at completion). Real Installer 43 passes fresh, reuse,
replacement and incomplete-data cases, including saved-file preservation and
native icon.library checks. The damaged-archive requester needs an interactive
dismissal check; its unattended fixture was stopped and is not a pass. The final
preferences-repair guard passes the native bad-options case: damaged preferences
stay in the backup and the repaired installation uses verified originals. Host
sanitizer tests pass, including the preferences checks.

No original bytes are embedded in installer source. Generated headers contain
names, coordinates, lengths, hashes and authored replacement instructions.
Native output is checked against independently generated host inputs.

## 68040 timing

MEASURED: the current `a1200-040` preset passes original-paired VBL and Time
Manager fixtures including restoration and rejection controls. Runs
`.run-K5MX0d` and `.run-Y340pU` retain the evidence under ignored `amiga/`.
The cadence run `.run-dS7sF6` completes its route and restoration, with 58
conversion passes over 58 updates and none outside frame end. Of 58 timer
intervals, 57 are five ticks and one is six. The strict cadence gate fails;
this is not 68040 release acceptance. The five-update profiling window is 21
PAL fields. This preset's MMU core does not establish real-hardware speed.

## Build and WHDLoad

Two final clean builds produce identical stripped executables. The bundled
converter exposed uninitialized section padding during an incremental/clean
comparison. AmigaXDev `ba5cc6c` supplies the pinned zero-padding fix; the checker
proves that section data, layout and relocation bytes are unchanged. The game
and installer pass that comparison, repeat conversion and altered-byte rejection.
The separately installed tool is selected by `ELF2HUNK`; shared setup integration
is recorded in item 19.9. The ordinary release
build is versioned 0.90 and passes the no-runtime-hash/trace audit. The installer
archive reproduces byte-for-byte and passes independent Lhasa extraction and
member-hash checks. Full standalone integration passes in
`tmp/integration-gate-8i3xpo2i`: title rendering, two completed gameplay routes,
restoration, original pixels/state and the no-right control. These runs predate
only the padding fix; the allocated data and relocation bytes are unchanged.
`make dist` includes `tools/check_release.py`; the JSON beside
the archive records its current size and SHA-256. Building is not publication.

The slave enables AGA and binds the executable to WHDLoad before entry. The
runtime installs only guest Line-A under WHDLoad, leaving WHDLoad's protected
real exception table alone. Standalone exception handling is unchanged. These
policies reuse Alone in the Dark through pinned AmigaXDev helpers.

MEASURED: the 040/MMU observation `tmp/whdload-release-vt22nnlp` reaches
`CORE / RUNNING` with no scaffold error, TC=$8000 and CACR=$80008000. The run
ends at WHDLoad's diagnostic timeout. This establishes neither gameplay nor
normal quit. PRELOAD on/off gameplay, save persistence, clean quit and hardware
validation remain required by item 14. A LoadSeg-triggered debugger attempt did
not connect under KickEmu; it is not a pass. A boot-level connection works,
but the installed MMU emulator rejects `monitor fields` (Rcmd protocol error).
The automated WHDLoad input/quit attempt was stopped; no completion is claimed.

The candidate includes complete LGPL installer sources and build instructions,
and excludes original game files, ROMs, Workbench and WHDLoad binaries.

## Hardware candidate checks

On the owner's A1200 68040/40, 32 MB, MMU:

- Install from the original SIT with Installer 43. Keep the installer messages.
- Launch using the supplied icon (PRELOAD), start with Space, and play several rooms.
- Save with Amiga-S, quit with Amiga-Q, relaunch and load with Amiga-O.
- Check music, effects, movement, and the F10 WHDLoad exit.
- Repeat launch/play/quit with PRELOAD removed from the icon tooltypes.
- Repeat with the WHDLoad MMU option enabled; retain any `.whdl_register` report.

These observations establish functionality on the stated hardware, not an exact
five-tick cadence match. The unresolved original-paired 040 cadence gate stays open.

## Candidate identity

`dist/PrinceOfPersia2-0.90.lha`: 181,660 bytes, SHA-256
`7eb6280f734180904efdecc4178e0b4b0cf0b81fd407a33635023207a588ca59`. This installer archive contains no original game data.
