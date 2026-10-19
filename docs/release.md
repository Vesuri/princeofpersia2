# Version 0.90 (19.10.2026)

The reported death/restart service defect is fixed and passes original-paired
regression checks. The owner confirms that guard-death/Enter restarts into
playable gameplay on the updated a1200-030 release. Physical-hardware testing
remains open in item 14. Building and auditing the archive is not publication.
The owner will test an A1200 68040/40 with 32 MB and MMU.

## Package

The installer archive contains the release executable, KickEmu slave, Installer
43 script, native extraction helper, icons and the helper’s LGPL licence.
The ReadMe links to the helper source on GitHub; source files are not bundled.
No original game data, ROMs, Workbench or WHDLoad binaries are included.
The packaged ReadMe follows the WHDLoad install template and contains only
player-facing information, with one history entry: initial release.
`manifest/release-template.json` pins the template used.

A clean rebuild reproduces the current stripped executable and LHA byte for byte.
Independent Lhasa extraction verifies the member list, hashes, version tags,
HUNK/icon headers and payload identities. Ordinary release audits exclude
runtime hashing, trap traces, profiling and test input.

The slave and helper retain the date-only changes qualified by
`tmp/item14-final/date-only.json`. The game contains the locked-handle dispatch
correction below and embeds its colour-transfer table. The Installer no longer
copies that table as a separate file.
Existing correctness evidence retains its stated scope.

## Installer

Host checks pass all 100 installed output hashes, idempotency, repair,
preservation of differing files and saves, damaged-archive rejection,
cancellation and staging cleanup. The native helper retains its bounded
4,096-byte stack contract. Installation supplies uncompressed files.

Real Installer 43 checks cover fresh installation, reuse, replacement,
incomplete data and damaged preferences. Every accepted case requires Installer
to return, the expected questions to be reached, all output hashes to match,
package files and native icons to validate, saved files and unrelated files to
survive, and staging to be removed. Scripted requester answers establish script
flow, not full interactive installation coverage.

The actual damaged-archive requester was dismissed by the owner in
`tmp/installer43-_plnd5zw`. Installer returned and every pre-existing installed
file remained unchanged. Its helper and script differ from the final package
only in version dates. This closes the interactive rejection gate; the earlier
unreached and timed-out attempts remain failures.

## Runtime qualification

Item 22's corrected-clock original-paired timing/service and matched-workload
performance checks pass on a1200-030 for the preceding release. The correction
below changes only MoveHHi error handling; those measurements are not new
measurements of this binary. See `timing.md`, `performance.md` and
`manifest/accelerated-clock.sha256`. Full 040/MMU emulator timing-service and
cadence checks pass; they establish emulator behavior, not calibrated 25 MHz
or physical-hardware performance.

The normal release startup matches the original first-text viewport without
runtime hashes or probes. Original-paired title/gameplay integration, input,
restoration, freed-handle guards and accepted gameplay coverage remain covered
by their existing manifests. Items 12 and 16 are accepted under D10–D12/D18;
68020 is unsupported under D17.

Release WHDLoad checks cover gameplay and normal Amiga-Q exit with PRELOAD on
and off, ordinary F10, save and fresh-boot load on 030/NOMMU, and save/load with
040/MMU and caches active. Normal cleanup is paired with Macintosh
`release-quit-4`, including audio close, window disposal, restored trap patches,
platform restoration and successful shell return. Loading must resume gameplay;
save forks and metadata must remain unchanged. Damaged-evidence controls retain
positive undamaged baselines.

040 observations use the explicitly enabled pinned cache-aware debugger.
Physical RAM alone can miss dirty copyback data; the controlled shared observer
fixture validates physical cache overlays without changing guest cache state.
This is a physical-address data observer, not logical MMU translation or an
instruction-cache view. `manifest/mmu-observer-evidence.sha256` retains that
control matrix. Ordinary play uses no debugger.

## Death/restart correction

MEASURED: the owner's guard death followed by Enter in level 1, room 15
reached `LoadCustomRoomData` at CODE 3+$13AC with a locked CUST handle.
The release observer (`amiga/.run-q91120`) captured `_MoveHHi` returning
`memLockedErr` (−117), which the dispatcher incorrectly made fatal.
System 7.5.5 (`ref/mame/runs/movehhi-locked-1`) returns that error in D0 and
MemError, sets N, and leaves the handle and payload unchanged in all four
locked/purgeable/resource state combinations. The original caller continues
with HLock.

Alone in the Dark already returns the shared allocator's result to the caller;
Vette's permanent-handle implementation needs no movement. This port now
returns the measured locked-handle result, retaining loud stops for unknown
handles and other unmeasured failures. `HeapMoveHigh.inc` is unchanged.
The opt-in quick-trap fixture exercises the actual restart trap through full
dispatch for all 32 incoming CCR values and four locked states, checking D0,
MemError, preserved registers/payload/state/free space and the return PC.
Release builds exclude that fixture and its counter.

MEASURED: all 128 locked-target cases fail before the dispatcher correction
(`amiga/.run-ewMWvi`) and pass afterward (`amiga/.run-HyZp8N`). The latter
also passes the existing quick-trap controls, completed integration route and
hardware restoration. The original checker rejects nine altered records.
Host tests, release link/audit checks and independent archive extraction pass;
two clean rebuilds produce identical game executables and archives
(`tmp/death-restart/rebuild.json`). The restart-fix package comparison confirms that only the game executable
changed in that correction. The current package additionally updates the
ReadMe installation instructions with the supplied download link and the
actual installation/reinstallation steps. The subsequent embedded-table change
updates the game and Installer and removes the separate table member.
`manifest/death-restart.sha256` pins the captured failure and regression evidence.

The owner confirms the requested guard-death/Enter retry succeeds. Separately,
the release observer in `amiga/.run-V5Wuli` records MoveHHi returning −117 and
then a completed restart frame (`D0.w=$FFFE`) in level 2, room 2. This directly
exercises continuation past the formerly fatal result; its recorded room is
not the original level-1/room-15 failure site. The immutable log snapshot and
release receipt are pinned in the same manifest. The release executable is
`a479b7e4250565b54b66a90ad0c31425e65bb810995d36014d0ccc17891b5913`.


## Embedded colour-transfer table

The pinned 256-byte video transfer is compiled from an ignored generated header
into the release executable. The package and fresh installation no longer need
`video-transfer.bin`; an old copy retained during an upgrade is unused. No
runtime read, allocation or hash scan remains for this table. The build rejects
corrupt or truncated input before generating its header, and release audits
check the embedded bytes and absence of the external-file dependency.

The a1200-030 release startup (`amiga/.run-MXi3Vo`) completes with no table file
on the staged volume and matches the pinned original opening viewport pixels.
Archive extraction and a clean rebuild pass. Evidence is pinned in
`manifest/embedded-transfer.sha256`. Diagnostic captures can still carry a
separate copy for their host-side colour comparisons.

## Hardware checks still required

On the owner's A1200 68040/40, 32 MB, MMU:

- Install from the original SIT using Installer 43.
- Launch from the supplied icon with PRELOAD and play several rooms.
- Check music, effects and movement.
- Save with Amiga-S, quit with Amiga-Q, relaunch and load with Amiga-O.
- Check the F10 WHDLoad exit.
- Repeat launch/play/quit with PRELOAD removed.
- Repeat with the WHDLoad MMU option enabled; retain any `.whdl_register` report.

The requested a1200-030 emulator play session is a user check, not a replacement
for this hardware gate.

## Current evidence and identity

`manifest/release14-current.sha256` pins the current package, build checks,
installer results, release gameplay captures and rejection-control reports.
The earlier `manifest/release14-evidence.sha256` retains its historical scope.

| Check | Completed run |
|---|---|
| installer-fresh | `tmp/installer43-7zc0om_n` |
| installer-reuse | `tmp/installer43-e2ytu814` |
| installer-replace | `tmp/installer43-sxrqtl5h` |
| installer-incomplete | `tmp/installer43-fpyawzj1` |
| installer-bad-options | `tmp/installer43-rwf4q51z` |
| 030-preload | `tmp/whdload-release-uknuj2iv` |
| 030-no-preload | `tmp/whdload-release-l7qq5rs4` |
| 030-f10 | `tmp/whdload-release-051iof4a` |
| a1200-030-save | `tmp/whdload-release-06v_2vge` |
| a1200-030-load | `tmp/whdload-release-gtfnq9eo` |
| a1200-040-save | `tmp/whdload-release-or4j5prc` |
| a1200-040-load | `tmp/whdload-release-u4fuh4kj` |

The hidden damaged-preferences attempt `tmp/installer43-adu5zuos` was
interrupted after copying stopped progressing; its cause is unestablished and
it is not a pass. A fresh visible retry completed all checks. The harness now
supports `--window` for observation without changing its scripted answers.
The sandbox launch failure and the run interrupted for the owner’s date change
also remain non-passing attempts.

`dist/PrinceOfPersia2-0.90.lha`: **166,782 bytes**, SHA-256
`0e17f73881d77c19a296e6c7762e33589f64d4fe3ddcce70303727fbb0810f83`.

Release executable SHA-256:
`860e4afb0c1e17c554653e7ae954fa084025317cb2bf1ee9c61d224a0e511b12`.
