# Version 0.90 (19.10.2026)

Software qualification is complete. Physical-hardware testing remains open in
item 14. Building and auditing the archive is not publication.
The owner will test an A1200 68040/40 with 32 MB and MMU.

## Package

The installer archive contains the release executable, KickEmu slave, Installer
43 script, native extraction helper, icons and complete LGPL helper sources.
No original game data, ROMs, Workbench or WHDLoad binaries are included.
The packaged ReadMe follows the WHDLoad install template and contains only
player-facing information, with one history entry: initial release.
`manifest/release-template.json` pins the template used.

Two clean builds produce identical stripped executables and byte-identical LHAs.
Independent Lhasa extraction verifies the member list, hashes, version tags,
HUNK/icon headers and payload identities. Ordinary release audits exclude
runtime hashing, trap traces, profiling and test input.

The game, slave and helper differ from their preceding builds only in the
16-to-19 October date strings; the Installer script is likewise date-only.
`tmp/item14-final/date-only.json` and `installer-script-date-only.json` establish
those identities. Existing correctness evidence retains its stated scope.

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
performance checks pass on a1200-030. The final date-only release retains those
runtime instructions. See `timing.md`, `performance.md` and
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

`dist/PrinceOfPersia2-0.90.lha`: **191,356 bytes**, SHA-256
`97cce957b7cfcb152ebdfbbadcb3513bf357768009d3e0d35a7c4e0b876bbefa`.

Release executable SHA-256:
`1880682905b4062b88b32a9059747dcc1412f86a3dc21ae840cc657f84fd7439`.
