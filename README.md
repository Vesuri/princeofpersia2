# Prince of Persia 2

Macintosh-to-Amiga port following Vette! and Alone in the Dark. The original 68k game
instructions run natively on an AGA Amiga with a 68020 or better, and the port
implements the Macintosh services they call. The display is PAL hires interlaced,
showing the original 512×384 window.

The release can be inspected and extracted reproducibly (see the source inventory).
A pinned headless Macintosh reference reaches the opening cutscene, with a verified
static/live map of that route. Paired service, audio and presentation probes exist.
Original native startup reaches the first playable room, with matching reference
pixels and state and verified gameplay input. Timing and full-game validation
remain open.

## Setup checks

AmigaXDev supplies the shared toolchain and tools through
`~/.local/share/amiga/env.sh`. Run `make check` in AmigaXDev to verify that installation.
Here, run `make check` to verify the staged archive and compiler access. For later
builds and runs, source `. amiga/env.sh` in the same shell. `AMIGA_ENV` can override
the shared environment file.

The supplied archive is stored read-only at `tmp/originals/Prince_Of_Persia2.SIT`,
which Git ignores. On a fresh checkout, place your own matching archive there and
run `make verify-input`. Do not add original or extracted game material to Git.

Run `make inspect` to reproduce the application survey, `make extract-original-data`
to create the verified fork/metadata layout under `tmp/runtime-data/`, and
`make check-extraction` for the repeatability and preservation gate. These commands
require `unar`, `lsar`, macOS `xattr`, and the shared Python environment with Capstone.
Differing existing output is retained under `.replaced/`; scratch stays in `tmp/`.

- File: `Prince_Of_Persia2.SIT`
- Size: 7,716,607 bytes
- SHA-256: `02437dc253dffec4eee516f8e5e0f2ecaccc4bd31f3d6957d42ea4f87b80b080`

Run `make reference` for the original System 7.5.5 / Macintosh IIci opening
checkpoint, or `make check-reference` for the two-run repeatability and rejection
gate. Local ROMs/system images and MAME must match the reference manifests. See
[Macintosh reference](docs/mac-reference-loop.md) for inputs, results, and limits.

See [source inventory](docs/source-inventory.md), [design](docs/design.md),
[open work](docs/open-work.md) and [agent rules](AGENTS.md).

Run `make map` for the static census, `make ghidra` for headless exports, and
`make static-map-check` for two fresh original runs plus rejection controls. See
[static and live map](docs/static-map.md) for coverage, service seams and limits.

Run `make` or `make release` for the playable release build. Runtime SHA-256
scans, trap tracing and expensive exit audits are off by default. `make VERIFY=1`
explicitly enables them; verification gates select that mode themselves. Both
modes retain file-format, bounds, ownership and original-byte guards. Host
installation/staging tools verify the pinned inputs before use.

Run `make play AMIGA_SETUP=a1200-040` for a visible interactive release run with
sound. The launcher refuses verification or probe builds. `make host-tests`
runs portable helper tests; `make scaffold` and `make scaffold-check` retain the
empty Amiga boot/quit checks. See [runtime scaffold](docs/scaffold.md).

Run `make core` for the bounded original startup executable or `make core-check`
for paired Macintosh/Amiga execution and rejection controls. See
[core runtime](docs/core-runtime.md) for relocations, services and current limits.

Run `make files-check` for paired original/native file operations, resource
validation and save persistence across a fresh boot. See [file services](docs/file-services.md)
for the tested scope and the distinction between service probes and continued gameplay.

Run `make display` to enable the display services or `make display-check` for
paired Macintosh contracts, repeatable title/first-room presentation and rejection
controls. Original native startup then stops at `InitFonts`; scene validation uses
display-service replay. See [display services](docs/display.md) for scope and limits.

Run `make ui` for VBI input and port-owned menus/prompts, or `make ui-check` for
paired original contracts, byte-verified protection/compression seams and native
output/rejection checks. The bounded service fixture stops at an explicit startup checkpoint after
144 matching original calls. See [input and UI](docs/input-ui.md) for controls,
coverage and the distinction between service fixtures and native gameplay.

Run `make audio` for the port-owned Halestorm driver or `make audio-check` for
paired original title/gameplay/death/cutscene audio events and Paula DMA checks.
See [audio](docs/audio.md) for measured voice priority, data provenance and the
service-replay boundary. Audio uses Paula voices without software mixing.

`make integration-contracts-check`
reproduces the original fixed-point, font, GWorld and title-copy comparisons;
`make integration-check` additionally runs the title-to-gameplay acceptance gate.
See [native integration](docs/native-integration.md) for the verified route,
shared services and current limits. `make audio` builds the ordinary executable
with all startup services; the integration gate adds test-only input and probes.

The current performance target is `a1200-030` with 8 MB Fast RAM. See
[performance measurements](docs/performance.md) for the explicit profiling build,
matched-workload checks and Kalms dirty-rectangle conversion at frame end.
Profiling is separate from ordinary release and the correctness fixtures.
