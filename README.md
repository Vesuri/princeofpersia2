# Prince of Persia 2

Macintosh-to-Amiga port following Vette! and Alone in the Dark. The original 68k game
instructions run natively on an AGA Amiga with a 68020 or better, and the port
implements the Macintosh services they call. The display is PAL hires interlaced,
showing the original 512×384 window.

The release can be inspected and extracted reproducibly (see the source inventory).
A pinned headless Macintosh reference reaches the opening cutscene, with a verified
static/live map of that route. The Amiga UI build executes 144 matching original
startup calls before its unsupported `Palette2CTab` boundary. Paired service and
presentation probes exist; end-to-end native gameplay remains unverified.

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

Run `make` for the empty host build and synthetic helper tests, `make amiga` for
the empty Amiga executable, or `make scaffold-check` for the full boot/quit and
rejection gate. See [runtime scaffold](docs/scaffold.md) for reuse boundaries,
launchers, build audits and development-only packaging.

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
output/rejection checks. Startup reaches the named `Palette2CTab` boundary after
144 matching original calls. See [input and UI](docs/input-ui.md) for controls,
coverage and the distinction between service fixtures and native gameplay.
