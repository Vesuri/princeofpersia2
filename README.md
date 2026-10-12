# Prince of Persia 2

Initial setup for a Macintosh-to-Amiga port following Vette! and Alone in the Dark.
The intended approach preserves the original game instructions and implements the
Macintosh services they call, subject to checking the supplied release later.

Only the repository, shared environment entry point and original archive staging
are ready. The archive has not been extracted or analyzed. No runnable port exists.

## Setup checks

AmigaXDev supplies the shared toolchain and tools through
`~/.local/share/amiga/env.sh`. Run `make check` in AmigaXDev to verify that installation.
Here, run `make check` to verify the staged archive and compiler access. For later
builds and runs, source `. amiga/env.sh` in the same shell. `AMIGA_ENV` can override
the shared environment file.

The supplied archive is stored read-only at `tmp/originals/Prince_Of_Persia2.SIT`,
which Git ignores. On a fresh checkout, place your own matching archive there and
run `make verify-input`. Do not add original or extracted game material to Git.

- File: `Prince_Of_Persia2.SIT`
- Size: 7,716,607 bytes
- SHA-256: `02437dc253dffec4eee516f8e5e0f2ecaccc4bd31f3d6957d42ea4f87b80b080`

See [source inventory](docs/source-inventory.md), [design](docs/design.md),
[open work](docs/open-work.md) and [agent rules](AGENTS.md).
