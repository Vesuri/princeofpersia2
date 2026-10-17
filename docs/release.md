# Version 0.90 (16.10.2026)

Item 14 is in progress. No release archive has been qualified or published.
The retained LHA candidate predates the conversion-bounds, buffer-synchronization and heap
changes; its measurements below describe that candidate.
The owner will test an Amiga 1200 with 68040/40, 32 MB and MMU. Hardware
confirmation remains required; emulator observations do not replace it.

Under owner decision D14, item 15 (68020 support) and remaining performance
items 19.8–19.9 are deferred beyond 0.90. The owner separately authorized
19.4, 19.5 and 19.7 ahead of release. The deferred items’ future acceptance gates
remain intact.
Item 14’s 040 cadence, hardware and installer gates remain
required before this candidate can be qualified.

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
dismissal check. The bounded Return-key attempt reaches the real rejection
requester but does not return; the visible attempt also remains incomplete
because Computer Use access to the emulator was not approved. Neither is a pass
(`tmp/installer43-akceplca`, `tmp/installer43-wetwdn3c`). The final
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
The current-runtime cadence run `.run-joH7V1` completes its route and restoration, with 58
conversion passes over 58 updates and none outside frame end. Of 58 timer
intervals, 57 are five ticks and one is six. All six altered-record controls reject, but the strict cadence gate fails;
this is not 68040 release acceptance. The ordinary release build is restored. The five-update profiling window is 21
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
`tmp/integration-gate-tadhjrhi`: title rendering, two completed gameplay routes,
restoration, original pixels/state and the no-right control. This gate includes
the normal-quit service changes; its 20 native-title, six reference-title and
19 gameplay evidence controls reject incomplete or damaged observations.
`make dist` includes `tools/check_release.py`; the JSON beside
the archive records its current size and SHA-256. Building is not publication.

The slave enables AGA and binds the executable to WHDLoad before entry. The
runtime installs only guest Line-A under WHDLoad, leaving WHDLoad's protected
real exception table alone. Standalone exception handling is unchanged. These
policies reuse Alone in the Dark through pinned AmigaXDev helpers.

MEASURED: ordinary release 040/MMU observations
`tmp/whdload-release-dhk447_d`, `tmp/whdload-release-x2vzwus5` and
`tmp/whdload-release-4jxr81ou` reach level 1, room 4, advance eight updates,
complete original ExitToShell and return to the shell with `Return OK.`.
Window disposal, restored trap patches and platform restoration are observed.
The first two runs' complete first/moved context captures match the accepted
030 route byte-for-byte. WHDLoad reports TC=$8000 and CACR=$80008000;
the effective machine has CPU/MMU 68040.

These runs do not establish their stricter audio-return observation: the guest
return probe is missed and an attempted native-return memory read is inconsistent.
Their failed strict reports remain failed. The cache-aware observer below now
passes fresh strict runs. The shared MMU playbook separately
requires positive startup, gameplay, file operations and normal exit, with the
effective machine and saved MMU state checked. The dedicated workflow below
covers that functional scope without claiming the individual audio result.

### MMU functional workflow

MEASURED: fresh 040/MMU release runs save
(`tmp/whdload-release-j1sbq4xw`) and load in a new boot
(`tmp/whdload-release-ce6kvhlq`). Both original file routines return success;
the load reaches a new level-1/room-4 first frame and a completed update. Both
complete ExitToShell, restore the platform and return successfully to the shell.
The save forks and metadata remain byte-identical after loading. The effective
CPU/MMU are 68040, JIT is disabled, compatible/data-cache emulation is active,
and WHDLoad records TC=$8000 and CACR=$80008000. These satisfy the functional
MMU scenario in AmigaXDev's `docs/playbooks/whdload-mmu.md`; they do not establish
cadence or real-hardware acceptance.

`--mmu-functional` is an explicit, separate observer scope. Its report is
`mmu-functional.json`, with `audio_return: not observed`; the paired cleanup
checker still rejects it without its required audio-return marker. The dialog
callback and original file-routine register returns are observed. In both MMU
runs the raw dialog ID reads as zero despite the callback and successful file
operation. The controlled debugger fixture below establishes that physical RAM
reads miss dirty copyback data. Fresh cache-aware runs observe the expected IDs
and audio returns. No game code or cache policy was changed to accommodate the
observer, and no failed strict result has been relabelled.

Reproduce with `tools/test_whdload.py --gameplay --setup a1200-040 --mmu
--mmu-functional --save-game --seconds 600 --ticks 24000`, then replace
`--save-game` with `--load-game-from SAVE_RUN`. Use the pinned debugger emulator.
`tools/check_whdload_mmu.py RUN --controls` checks the result and rejects eight
alterations (translation/cache state, CPU, JIT, NoMMU option, shell result,
restoration and file completion). The separate save/load corruption controls
also pass. Original-paired service cleanup remains covered on 030/NOMMU.

### Debugger memory coherence

MEASURED: the authored AmigaXDev `CacheObserver.s` fixture reproduces stale
Barto debugger reads with 68040 MMU and copyback data caching enabled. CPU
register loads contain the expected values while the stock debugger reads the
old physical RAM bytes. This establishes the observation defect independently
of the original game. The separate MMU write-attribute core fix was already
present in both emulator builds.

The opt-in `monitor memory cache` mode overlays valid physical data-cache bytes
without flushing, invalidating, allocating or writing any cache line. Misses
read the original byte address; `monitor memory physical` restores raw reads.
This is a **physical-address observer**, not logical MMU translation or an
instruction-cache view. Its validated scope is the 68040 and the fixture/game's
identity-mapped data addresses. GDB's host register cache must be refreshed after
`monitor fields`; this does not flush the guest cache. Debugger writes remain
unsupported and are not used to stimulate these tests.

| Controlled configuration | Stock/raw reads | Cache-aware reads |
|---|---|---|
| MMU on, data cache on | Stale in both observation phases | All 96 authored bytes match |
| MMU on, data cache off | Match | Match |
| MMU off, data cache on | Match | Match |
| MMU off, data cache off | Match | Match |

Two corrected repeats of each configuration pass in
`tmp/mmu-observer/accepted`; a four-run stock baseline is in
`tmp/mmu-observer/baseline-matrix`. Each run checks actual TC/CACR and effective
emulator configuration, CPU-loaded register values, a changed second value,
uncached Chip RAM, untouched bytes, cache misses and a longword crossing a line
boundary. Raw RAM stays byte-identical across each cache-aware observation.
Compiled continuation checks and normal WHDLoad/shell return establish positive
completion. All 64 evidence-corruption controls reject their damaged inputs.
Wrong emulator/compiler pins and an empty matrix are rejected before launch.

WHDLoad NOMMU itself leaves caches disabled. The authored fixture explicitly
sets the requested CACR during setup, then restores WHDLoad's prior value only
after the final observation and native completion checks. No flush or cache
policy change occurs inside the observation window. This experimental setup is
not a change to the game's cache policy.

Fresh ordinary-release MMU runs save (`tmp/whdload-release-lksg3pys`) and load
in a new boot (`tmp/whdload-release-4w5qb7iq`) with cache-aware observations.
The actual dialog IDs are 7001 and 7002, original file routines return success,
the persisted files match, and loading resumes gameplay. Both observe the
original audio-return selector 8/result 0, window disposal, restored patches,
ExitToShell and normal shell return. Both pass `check_release_quit.py` against
`ref/mame/runs/release-quit-4`, including six rejection controls each; save/load
corruption controls pass (three/five). The quit-control harness now retains each
scenario's options and requires its undamaged copy to pass before sabotage.
The prior failed strict reports remain failed.

The shared patch, authored fixture and runner are published in AmigaXDev
`codex/cache-observer`, commit `20bf49f`; main's existing dirty files were left
untouched. `manifest/mmu-observer-shared.sha256` pins these sources relative to
that checkout, `manifest/mmu-observer-inputs.json` pins the fixture dependencies,
and `manifest/mmu-observer-evidence.sha256` pins the captures and reports.
`manifest/release-debug.sha256` retains both emulator identities. The dedicated
installed build is `fs-uae-cache-observer` under `AMIGA_SHARE`.

Reproduce the controlled matrix after verifying the shared checkout's manifest:

```sh
. amiga/env.sh
(cd "$AMIGAXDEV_SOURCE" && shasum -a 256 -c "$OBSERVER_SHARED_MANIFEST")
python3 "$AMIGAXDEV_SOURCE/scripts/test_cache_observer.py" \
  --emulator "$AMIGA_SHARE/fs-uae-cache-observer/fs-uae" \
  --sha256 "$(awk '$2=="fs-uae-cache-observer" {print $1}' manifest/release-debug.sha256)" \
  --input-pins manifest/mmu-observer-inputs.json --work-dir tmp/cache-observer-repeat
```

Set `AMIGAXDEV_SOURCE` to that shared checkout and `OBSERVER_SHARED_MANIFEST` to
this repository's absolute shared-manifest filename. The shared runner also
accepts `--baseline --repeats 1` with the stock binary and its pinned hash.
For game observations, set
`FSUAE_WHDLOAD="$AMIGA_SHARE/fs-uae-cache-observer/fs-uae"` and run
`tools/test_whdload.py --gameplay --setup a1200-040 --mmu --cache-memory
--save-game --seconds 600 --ticks 24000`; replace `--save-game` with
`--load-game-from SAVE_RUN` for the fresh boot. `--cache-memory` is explicit;
ordinary play and the existing functional-only observer retain their behavior.

Host tests pass. The ordinary release executable remains byte-identical at
`c913cb3947c50d1cfe99297bec4636953ce9a7b93b7236101cdd62f8c6641247`.
These results establish debugger observation and functional correctness;
68040 cadence and real-hardware acceptance remain open in item 14.

### Indirect low-memory store

MEASURED: an unattended 030/NOMMU WHDLoad run stopped in Exec's cache flush
with a privilege violation (`tmp/whd-smoke-21zy7l81`). The write watchpoint in
`tmp/whd-smoke-jevm_h7_` identifies CODE 20+46B4, `waveObj::resetLoop`: it
stores through a null channel pointer to address $20. This overwrites the
Amiga privilege vector. The original Macintosh demo in
`ref/mame/runs/release-wave-demo` completes and records the same operation:
38 null-channel stores and two stores to allocated channels. The earlier
cutscene-only reference `release-wave-null` never reaches this store and is
not coverage for it.

The byte-guarded relocation redirects this instruction through a private service.
A null channel writes to the existing virtual Macintosh low-memory area;
non-null destinations retain their address and require a valid extent. The
service preserves registers and MOVE.L condition-code behavior. Unknown callers
and invalid extents remain loud stops. The installation recipe regenerates the
new relocation input; its SHA-256 remains pinned. Vette! and Alone in the Dark
have no matching indirect-store implementation to reuse.

The ordinary release store check `tmp/whdload-release-26s3nkpm` reaches the original
continuation after writing $FFFFFFFF to virtual address $20. The Amiga vector
is unchanged, and N/Z/V/C are 1/0/0/0. `tools/check_indirect_store.py` pairs that
capture with the completed Macintosh demo; deliberately damaged vector and status captures are
rejected. This is a store-correctness gate, not WHDLoad gameplay or quit acceptance.

The changed runtime passes the full original-paired integration gate described
above. Host tests and installation recipe tests pass with the updated input.
The rebuilt archive also passes a fresh native Installer 43 run
(`tmp/installer43-yw0udxgb`): all 100 installed inputs match their pins and
the installed icon passes native checks. A clean rebuild reproduces the archive
byte-for-byte.

Reproduce with a release build and `tools/test_whdload.py --setup a1200-030
--store-reference ref/mame/runs/release-wave-demo --seconds 1200 --ticks 24000`. Set
`FSUAE_WHDLOAD` to the separately installed `fs-uae-pop2-release/fs-uae` under
`AMIGA_SHARE`; `manifest/release-debug.sha256` pins it. It was built with the
canonical AmigaXDev build script and current core, frequency and debugger patches;
other installed emulators are unchanged. The reference trace is opt-in with
`POP2_WAVE_NULL_TRACE=1`, `POP2_AUDIO=route`, `POP2_DISPLAY_ROUTE=1`,
`POP2_NATURAL_OPENING=1`, and `POP2_DEMO_ROUTE=1` through `tools/run_reference.py`.
The debugger fixture observes the real instruction's continuation and requires
positive evidence; an unreached breakpoint is a failure.

The candidate includes complete LGPL installer sources and build instructions,
and excludes original game files, ROMs, Workbench and WHDLoad binaries.

### Normal quit

MEASURED: release build `d294cddfd4fdc8a70bfc87e90e09a193da51b0a112af6d4655dfa594e4870ca5`
passes gameplay and Amiga-Q under 030/NOMMU WHDLoad with PRELOAD on
(`tmp/whdload-release-jendvesz`) and off (`tmp/whdload-release-n0wnh06z`).
Both reach level 1, room 4, advance from update 1 to 9, complete original
cleanup and return to the shell with WHDLoad's `Return OK.`. COREDUMP writes
a register report even on successful exit; the checker inspects its result.
F10 also returns successfully after gameplay with ordinary WHDLoad settings
(`tmp/whdload-release-upuee4hq`), with no TIMEOUT option. Reproduce using
`--gameplay --quit-key --seconds 1200`. The Expert/CoreDump F10 variant reports
`DEBUG caused` and is not the accepted ordinary-exit test.

`ref/mame/runs/release-quit-4` supplies the paired Macintosh Command-Q route
and positive return to Finder. `manifest/quit-reference.sha256` pins its trace
and window captures. The port now implements disposal of its owned window,
including the Window Manager current-port fallback; audio selector 8 closes
playback using Alone in the Dark's policy and the existing voice/cache helpers;
restoring saved built-in trap addresses removes the runtime patches. ExitToShell
publishes and closes remaining resource files using the shared resource directory,
then returns through platform restoration. Shell success requires this explicit
normal exit and successful restoration. Other service stops and diagnostic
completion stops still return failure.

`tools/check_release_quit.py` checks both Pascal stack cleanup and the original
port rectangles, audio-close result 0, restored trap patches, original ExitToShell,
restoration flags and shell status. Missing completion, WHDLoad error, failed
shell status and altered-port controls are rejected. These are release binaries;
observation uses debugger breakpoints and guest keyboard input.

Reproduce the original with `POP2_DISPLAY_ROUTE=1 POP2_QUIT_REFERENCE=1
python3 tools/run_reference.py --name RELEASE_QUIT --deadline 600`. Reproduce
the native route with `python3 tools/test_whdload.py --gameplay --seconds 1200
--ticks 24000`, optionally adding `--no-preload`, using the pinned debugger
emulator described above. Pair its output with `python3 tools/check_release_quit.py
ref/mame/runs/release-quit-4 NATIVE_RUN --controls`.

### Save persistence

MEASURED: the ordinary release saves `Level 1` at the existing original-verified
idle checkpoint (`tmp/whdload-release-yexl3ytj`), then exits normally. A fresh
WHDLoad boot (`tmp/whdload-release-3asjabw1`) loads those files, reaches a new
level-1/room-4 first frame and a completed gameplay update, then exits normally.
The 100-byte data fork, empty resource fork and metadata remain byte-identical
after loading. Type/creator match the pinned original save fixture. This is a
level checkpoint, not an exact-position save. Saving immediately after Right
movement reaches an error dialog; the accepted fixture uses the original's idle
save point instead of changing that game decision.

`tools/whdload_save.py` requires original routine success, resumed gameplay,
valid metadata and unchanged persisted bytes. Three save and five load evidence
controls reject missing completion, truncated/corrupt files and a missing resumed
update. Reproduce with `tools/test_whdload.py --gameplay --save-game`, followed
by `--gameplay --load-game-from SAVE_RUN`, using the pinned debugger emulator.
Add `--seconds 600 --ticks 24000` to both. Check either run with
`python3 tools/whdload_save.py RUN --controls`.

## Hardware candidate checks

On the owner's A1200 68040/40, 32 MB, MMU:

- Install from the original SIT with Installer 43. Keep the installer messages.
- Try a damaged **copy** of the SIT; confirm the error can be dismissed and the existing installation stays intact.
- Launch using the supplied icon (PRELOAD), start with Space, and play several rooms.
- Save with Amiga-S, quit with Amiga-Q, relaunch and load with Amiga-O.
- Check music, effects, movement, and the F10 WHDLoad exit.
- Repeat launch/play/quit with PRELOAD removed from the icon tooltypes.
- Repeat with the WHDLoad MMU option enabled; retain any `.whdl_register` report.

These observations establish functionality on the stated hardware, not an exact
five-tick cadence match. The unresolved original-paired 040 cadence gate stays open.

## Candidate identity

`dist/PrinceOfPersia2-0.90.lha`: 182,823 bytes, SHA-256
`a9c3854a4b7f9ce6eff96fab26449e0d71f57f6532d0776580252739ad88be70`. This installer archive contains no original game data.
