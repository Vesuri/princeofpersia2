# Runtime scaffold

The empty host and Amiga programs build. The Amiga executable boots from the
Shell, requires PAL AGA and a 68020, takes over VERTB for four fields, restores
hardware/OS state and returns. It does not read original game files or execute
Macintosh instructions. The separate [core configuration](core-runtime.md) runs original startup and identity fixtures.

## Reuse boundary

`manifest/references.tsv` pins Alone in the Dark and the canonical AmigaXDev
framework. `manifest/scaffold-sources.tsv` records upstream paths, commits and
source hashes for copied files. These are source hashes before the documented
adaptations, not checksums of the local edited files.

The portable `src/mac/` helpers cover heap management, fork metadata and bounded
resource/file models, byte-pixel and region operations, palettes, windows and
UI record parsing. Prefixes are changed to POP2. Heap profiling hooks are removed;
the virtual volume has a neutral name and no Alone Movies exception. Most bounded helper capacities are inherited limits; the aggregate resource
capacity is raised to cover the measured multi-file lookup chain. The core
configuration wires heap, file and resource services into trap dispatch; see
[file services](file-services.md) for paired coverage. Display and UI helpers
remain unwired. Unsupported operations remain errors.

The 11,482-line donor MacLoader mixes reusable services with original offsets,
identity answers, gameplay patches and probes. Its bounded replacement is `CoreRuntime`, documented separately. The donor's low-memory patch tables,
resource-service integration, captured Times glyphs/metrics, embedded resource
fork, game routes and scene hooks are omitted. The audio driver has game-specific
voice counts and driver semantics and is reserved for remeasurement in item 10.
The 320×200 screen/lores adapter is omitted in accordance with D3/D4.

The Amiga platform retains the donor's library lifecycle, takeover/restore order,
Workbench startup-message discipline and GCC runtime shims. Its empty VBI does
only acknowledgement and counting. The empty configuration installs no keyboard, audio or Mac
trap handlers. The dA JoRMaS framework is unchanged from the pinned canonical tree;
see `src/platform/amiga/framework/UPSTREAM.md`. The unchanged public-domain Kalms
converter is retained as source; its 512×384 adapter belongs to item 8.

The shared CRT's `_start` discards main's return code. The local entry preserves
that code across constructor/destructor dispatch; the unused shared entry is
renamed and discarded by the linker. Allocation shims use `-fcheck-new`, and a
pure-virtual call raises ILLEGAL. No game bytes or generated original tables are
linked or tracked.

## Build and checks

- `make` / `make host-tests`: empty host program plus 18 inherited helper fixtures and a SHA/identity fixture.
  Tests use synthetic data only: heap compaction, resource mutation/serialization,
  failure atomicity, bounded reads/writes, metadata, clipping and record handling.
- `make amiga`: empty HUNK executable, `amiga/out/PrinceOfPersia2.exe`.
  Header dependencies and a content-checked flags file prevent stale objects.
- `make scaffold-check`: host tests, Amiga build and the complete live gate.
- `amiga/run.sh`: the same bounded boot/quit path used by the gate.
  `amiga/diag_run.sh` sets its deadline; `amiga/debug.sh` selects a read-only GDB
  observer and a background window. Custom observers must still produce the
  positive scaffold record and let the Shell resume to pass.
- `make slave`: builds the adapted KickEmu source with AGA/68020 requirements and
  `WHDLF_EmulLineA`. Game-specific resload/input block scanning is removed.
- `make dist-scaffold`: deterministic development-only LHA containing the empty
  executable, slave and ReadMe. No game installer is claimed or supplied.

`manifest/scaffold-inputs.tsv` pins the shared compiler/assembler/link tools,
SDK, CRT, emulator, ROM, SetPatch, launcher, slave includes and packaging helpers.
Verification precedes use. Paths are resolved through `amiga/env.sh`; overrides
must supply matching pinned inputs. The build audits the HUNK/version, rejects
soft-float helpers, retains every debugger data probe and partial-links all
compiled modules without garbage collection to find dormant missing bridges.
A build receipt binds diagnostic flags to executable and ELF hashes. Diagnostic
builds cannot be packaged.

Each run owns a fresh ignored `amiga/.run-*` directory with private disks/state,
frozen executable/ELF, their hashes, logs and GDB output. The shared helper records
and stops only the project-owned emulator PID. A checkout lock prevents overlapping
runs; a busy debugger port fails without killing its owner. The empty configuration stages no original data. A fresh directory prevents stale Shell markers or saved state passing.

## Measured acceptance and limits

MEASURED in FS-UAE: A1200, 68020, PAL AGA, 2 MB Chip and 8 MB Fast, no JIT.
The effective private `debug.uae` is checked. This is a scaffold test configuration,
not a measured final Fast RAM requirement or a performance result.

The GDB observer must stop at the exact restoration checkpoint with stage 3,
no error, at least four owned VBI calls, matching saved/current DMA and interrupt
masks, original View and original VERTB vector. After detaching, the Shell must
write its success marker following the executable's return. A checkpoint alone
cannot prove a clean return. The observed initial run had four fields, DMA mask
720 and interrupt mask 24620; the gate compares saved state rather than hard-coding
those OS-dependent masks.

The gate repeats the positive path, tests a real failing main return through the
CRT/Shell, times out a never-completing observer, and rejects missing/partial or
failed evidence. It rebuilds the default executable after the failure control.
The archive is built twice and independently decoded with Lhasa; every member
must equal the explicit source payload. Generated output must remain ignored.
Reports stay under `tmp/scaffold-gate-*` and emulator logs under `amiga/.run-*`.

This gate establishes the empty Shell lifecycle only. Workbench-message handling
is inherited but not exercised here. Original game/service correctness, the game
display, audio, WHDLoad execution/MMU behaviour, installer, final release and real
hardware acceptance remain at their existing gates in `docs/open-work.md`.
