# Source inventory

The owner supplied `Prince_Of_Persia2.SIT` as the Macintosh Prince of Persia 2 archive.
The owner-provided download reference is
[Macintosh Repository](https://www.macintoshrepository.org/download.php?id=4673).
The local file was used; the download reference was not independently verified.

Evidence below comes from listing the archive with `lsar`, a scratch `unar` extraction
in ignored `tmp/`, resource-map parsing, linear disassembly with `m68k-amiga-elf-objdump`
and AmigaXDev's flow-following `amigaxdev.mac.m68k_sweep`. Linear trap counts are an
upper bound until a flow-following census replaces them.

## Archive

| Property | Value | Evidence |
|---|---|---|
| Archive filename | `Prince_Of_Persia2.SIT` | MEASURED |
| Size | 7,716,607 bytes | MEASURED |
| SHA-256 | `02437dc253dffec4eee516f8e5e0f2ecaccc4bd31f3d6957d42ea4f87b80b080` | MEASURED |
| Local copy | `tmp/originals/Prince_Of_Persia2.SIT` (ignored, read-only) | MEASURED |
| Format | StuffIt 5, Arsenic method; no nested disk image or installer | MEASURED |
| Contents | folder `Prince of Persia 2` with the application and a `Data` folder of 31 files | MEASURED |

The checksum is pinned in `manifest/originals.sha256`. The supplied file is untouched.

## Application

| Property | Value | Evidence |
|---|---|---|
| Name, type, creator | `Prince of Persia 2`, `APPL`, `PRNc`; resource fork only, dated 1995-05-23 | MEASURED |
| Version | `vers` 1 = 1.0; About box: © 1993, 1994 Jordan Mechner, published by Brøderbund | MEASURED |
| Architecture | 68k only; no `cfrg` | MEASURED |
| Segments | CODE 0 plus 23 segments; CODE 12–14 (Editor 1–3) are 4-byte stubs | MEASURED |
| A5 world | 8,176 bytes above A5, 22,266 below; 1,018-entry jump table at A5+32, all in unloaded form | MEASURED |
| Model | near-model segment headers; no CREL resources | MEASURED |
| Runtime | CODE 1 (1,058 bytes) with `DATA` 0, `ZERO` 0, `DREL` 0, `_STD`, `_STI`, `STRS`; THINK/Symantec family like Alone in the Dark | INFERRED |
| Symbols | 1,032 length-prefixed MacsBug name records immediately following RTS, including C++ (cfront-style) names; conservative subset | MEASURED |
| CPU | 216 68020-only instruction candidates on statically followed paths (CODE 5, 11, 16, 17, 21, 22); 74.3% coverage | MEASURED |
| `SIZE` | flags `$4A80` (suspend/resume, MultiFinder aware, front clicks, 32-bit compatible; not high-level-event aware); 2,560,000 minimum, 3,072,000 preferred in SIZE 0/1; SIZE −1 prefers 2,560,000 | MEASURED |
| Stated requirements | DITL 402: System 6.0.7 or higher, 256 colours, 32-Bit QuickDraw; DITL 602: Virtual Memory off | MEASURED |
| Game window | WIND 400, 512×384 content, procID 4 | MEASURED |
| Sound | Halestorm SoundMusicSys: `MDRV` 11 "MIDI Synth 3.45 4/27/94", `SMOD` 0–3, 34 `INST`, 34 `snd `; `Jnth` and `MDRV` resource requests sit beside `DecryptHandle`, the seam Alone in the Dark replaces | MEASURED |
| Copy protection | `CopyProtectionDialog`; `COPY` resource in `Prince.rsrc` | MEASURED |
| In-place packing | "Compress Data" menu item and a startup offer (`PackAllData`, `LZPackDataFile`, `PackDataAtStartUp`) rewrite `Data` files | MEASURED |

### Service surface

Selectors were read from the instruction loading D0 before each trap.

| Trap | Use | Evidence |
|---|---|---|
| `_QDExtensions` | NewGWorld, LockPixels, UnlockPixels, DisposeGWorld, GetGWorld, SetGWorld, GetPixBaseAddr (20 sites); no GetGWorldPixMap | MEASURED (linear) |
| `_HighLevelFSDispatch` | selectors 1, 4, 5, 7, 8, 9 (FSSpec calls) | MEASURED (linear) |
| `_SoundDispatch` | MIDI Manager (selector 4, client `MPLR`), SndSoundManagerVersion, Sound Manager 3 default output volume | MEASURED (linear) |
| Other | `Gestalt`, `SysEnvirons`, `_SwapMMUMode` (16), Time Manager, `VInstall`, `_PaletteDispatch`, SANE `Pack4` | MEASURED (linear) |

The program targets System 6 with feature checks; whether it selects FSSpec, Sound
Manager 3 or MIDI Manager paths from `Gestalt` answers is INFERRED and must be measured
on the reference. Alone in the Dark's System 7 dependency (`GetGWorldPixMap`) is absent.

## Data

| Files | Contents | Evidence |
|---|---|---|
| `Prince.rsrc` | `LEVL` 14, `SEQS` 236, `SHAP`, `CTBL`, fonts, `TEXT`, `COPY` | MEASURED |
| Level files (`Rooftops`, `Ruins`, `Caverns`, `Desert`, `Temple`, `Final`) | `SHAP`, `CTBL`, `PIEC`, `CUST`, `SHPL` | MEASURED |
| Character files (`Kid`, `Guard`, `Skeleton`, `Head`, `Birdhead`, `Jinnee`, `FlameSword`, `Horse`) | `SHAP`, `CTBL`, `FRAM`, `AFRM`, `SHPL` | MEASURED |
| Cutscene files (`NIS`, `NISTree`, `TNIS1`–`3`, `TempleNIS`, `ThroneNIS`, `TowerNIS`, `Title`, `Happy`) | `SHAP`, `CTBL`, `SCRP`, `ANI ` | MEASURED |
| `DigiSnd.dat`, `MIDISnd.dat`, `NISDIGI.dat`, `NISMIDI.dat` | data-fork Mohawk `MHWK`/`RSRC` archives (`WAVE`, `Cue#`, …) | MEASURED |
| `Mohawk.ini` | audio configuration text: 4 MIDI voices, 1 wave voice, low resolution, no interpolation | MEASURED |
| `Prince2.opt` | `DSLV` resource and a 16-byte data fork | MEASURED |

`SHAP` headers start with a flags word followed by row bytes, width and height.
Full-screen images are 510–520 pixels wide and 365–384 lines tall. INFERRED

## Reproduction

Run `make inspect` to unpack a fresh copy under ignored `tmp/`, verify the archive
and all fork/metadata hashes, parse resources through `amigaxdev.mac.resource_fork`
and repeat the existing CPU survey through `amigaxdev.mac.m68k_sweep`. The CPU
classifier runs its positive and negative self-tests first. Its traversal starts
from sorted CODE 0 jump-table entries, as in the shared sweep. Static coverage is
not execution evidence; unresolved transfers are reported. Procedure-name counting
accepts length-prefixed records immediately after word-aligned RTS instructions;
it is a conservative subset, not a complete symbol census. Printable strings alone
are not evidence of procedure names.

`make extract-original-data` writes `tmp/runtime-data/`. Every original file,
including the application, has its data fork at its original name (empty when
absent), a raw resource fork at `name.rsrc`, and a 32-byte `name.finfo` record.
Thus `Data/Prince.rsrc.rsrc` is the resource fork of `Data/Prince.rsrc`.
Unlike Alone in the Dark's application-only resource-at-base-name convention, the
application uses the same explicit fork layout as the data files here.

The extractor adapts Alone in the Dark's AppleDouble reader and AFI1 metadata
encoding. StuffIt 5 entries come directly from `lsar`; there is no HFS image,
installer, or classic StuffIt header to patch. Finder bytes come from `unar` and
are checked against `lsar` type/creator/flags. Original dates come from `lsar` in
UTC, converted to Macintosh epoch seconds, never from host filesystem timestamps.
`AFI1` contains the original 16-byte Finder record, creation/modification times,
and an FNV-1a checksum, as in Alone in the Dark.

`manifest/runtime-data.sha256` pins all 96 output files relative to the runtime
data directory. `manifest/mac-helpers.sha256` pins the shared helper source files
relative to `AMIGA_PYLIB`; these are checked before import. Archive verification
precedes unpacking; complete output verification precedes installation. Identical
existing output stays untouched. A differing tree, including any extra saves,
is moved intact under ignored `.replaced/` before the verified tree is installed.
Scratch extractions remain in ignored `tmp/extract-*/`.

`make check-extraction` checks two independent extractions against the manifest,
including a changed caller timezone, rejects altered inputs and malformed fork
extents, checks preservation of identical and differing destinations, and verifies
that all extracted paths are ignored and no scratch/replacement paths are tracked.
The gate completes with `EXTRACTION GATE PASS`. `git status --ignored` supplies the
repository-level tracking check. No game-derived output has been committed.

The measured System 7.5.5 / Macintosh IIci execution, identity answers, startup
branches and reproducible opening checkpoint are documented in
[the Macintosh reference loop](mac-reference-loop.md). Static facts above remain
distinct from that bounded execution evidence.
