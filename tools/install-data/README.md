# Pop2InstallData

Standalone C99 extractor for the pinned Prince of Persia 2 Macintosh StuffIt 5
archive. The plain archive needs no HFS or InstallerMaker decoding. Original
inputs remain read-only. All 96 forks and AFI1 metadata records plus four regenerated runtime inputs are checked by
SHA-256 before publication. Existing differing files cause a named failure;
matching files and unrelated saves stay untouched. A failed attempt removes
only its own staged and newly published files.

The decoder, SHA-256, platform I/O and startup are canonical AmigaXDev sources
extracted from Vette! and Alone in the Dark, pinned in
`manifest/install-shared.sha256`. `install.c` supplies this game's manifest and
publication policy. `generate_manifest.py` emits names, sizes, offsets and hashes
only. It verifies the original archive and independent host extraction first.
No game bytes are embedded in the helper.

Source `amiga/env.sh`, then run `make -C tools/install-data amiga` for the
freestanding Amiga executable or `make -C tools/install-data test` for the host
extraction and preservation suite. The Amiga executable has no unresolved
runtime helpers. Native stack and installation tests remain required before
release. The helper regenerates relocations, the file catalogue, UI seams and audio
pitch from the verified application. The separately measured video transfer
input is still supplied by release setup.

These helper sources are LGPL-2.1-or-later; see COPYING.LIB. The Arsenic and
StuffIt 13 tables retain MacPaw/XADMaster's copyright notices. The game executable
is separate from this installer.
