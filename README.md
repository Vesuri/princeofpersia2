# Prince of Persia 2

Macintosh-to-Amiga port following Vette! and Alone in the Dark. The original 68k game
instructions run natively on an AGA Amiga with a 68020 or better, and the port
implements the Macintosh services they call. The display is PAL hires interlaced,
showing the original 512×384 window.

The release can be inspected and extracted reproducibly (see the source inventory).
A pinned headless Macintosh reference reaches the opening cutscene, with a verified
static/live map of that route. No runnable
Amiga port exists yet.

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
