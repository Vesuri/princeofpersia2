# Version 0.92 (31.10.2026)

Version 0.92 adds port-1 joystick input (`docs/input-ui.md`) and shows the
game's stop reason in the WHDLoad requester. It changes no other service.
By owner decision, 0.92 runs only the checks for what changed since 0.91; the
0.91 evidence below keeps its stated scope for the unchanged code.
Physical-hardware testing remains open in item 14. The audited archive is
attached to the GitHub release 0.92.

## Version 0.92 checks

All passed on commit `a250d7a` and on one executable:

- Host scaffold tests, including `tools/test_amiga_joystick.cpp`
  (directions, buttons, key positions and the Space/Ctrl/Shift fire choice).
- `make dist`: reproducible archive and independent audit.
- `tools/test_whdload.py --gameplay --stop-reason` on a1200-030. The run
  presses an unmapped raw key in play. It requires the named stop, restored
  system state and the requester text "Prince of Persia 2 stopped: INPUT /
  UNMAPPED RAW KEY (trap $A975 at CODE 17+$303C)" in the WHDLoad result, with
  no "could not start" text.
- Nine `--route-input` runs and `tools/check_joystick.py`: 4
  joystick/keyboard pairs and 5 distinguishing controls.

# Version 0.91 (27.10.2026)

Version 0.91 fixes the out-of-memory stop of WHDLoad report 7036, which also
needed the original's heap placement and relocation, adds the two
services the credits page needs, and replaces the renaming resource-fork
publication that reset Kickstart under WHDLoad; all are described below. The 0.90
death/restart correction remains, with the owner's confirmation that
guard-death/Enter restarts into playable gameplay on a1200-030.
Physical-hardware testing remains open in item 14. The audited archive is attached to
the GitHub release 0.91. The owner will test an A1200 68040/40 with 32 MB
and MMU.

## Package

The archive uses the drawer `PoP2 Install` and icon `PoP2 Install.info` so the
icon name fits a 30-character filesystem limit.
The installer archive contains the release executable, KickEmu slave, Installer
43 script, native extraction helper, icons and the helper’s LGPL licence.
The ReadMe links to the helper source on GitHub; source files are not bundled.
No original game data, ROMs, Workbench or WHDLoad binaries are included.
The packaged ReadMe follows the WHDLoad install template and contains only
player-facing information, with history entries for 0.90 and 0.91.
`manifest/release-template.json` pins the template used.

A clean rebuild reproduces the current stripped executable and LHA byte for byte.
Independent Lhasa extraction verifies the member list, hashes, version tags,
HUNK/icon headers and payload identities. Ordinary release audits exclude
runtime hashing, trap traces, profiling and test input.

MEASURED against the 0.90 archive (`tmp/release-091`): the slave, Installer
script and helper differ only in their version strings, and the icons and
licence are unchanged. The game adds the display-memory, credits-page,
heap-placement and in-place publication changes below to 0.90's locked-handle dispatch correction and embedded
colour-transfer table. The Installer no longer copies that table as a
separate file.
Existing correctness evidence retains its stated scope.

## Zorro III pointer regression

The owner’s missing-artwork/red-rectangle report reproduced with WHDLoad
PRELOAD and 32 MB Zorro III memory. JIT was disabled in that failed run.
MEASURED: the release passed `$41C27330` to `StripAddress`; its native body
applied `$00FFFFFF`, redirecting the pointer outside the allocation. The
reported run later raised a privilege violation. The observed bad pointer
explains the graphics failure; the final exception alone did not identify it.

`StripAddress` now preserves native 32-bit pointers, following Alone in the
Dark’s existing service. Vette’s runtime was also inspected; it has no matching
service body to reuse. No new shared implementation is needed for this identity
operation. Macintosh handle flags are stored separately by the native heap;
the emulated address-mode byte does not limit Amiga physical addresses.

The original System 7.5.5 fixture checks 192 pointer-tag/CCR combinations.
Its 24-bit result is the Macintosh logical address; native callers must retain
the relocated physical address. The native fixture exercises both full and
warm dispatch, all 32 CCR inputs, both emulated address modes, zero, low and
high pointers. It also requires unchanged other registers and mode, and the
original-paired CCR and PC results.

Reproduce the original contract with `POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1
POP2_STRIP_ADDRESS=1 python3 tools/run_reference.py`; validate its completed
run with `tools/check_strip_address.py`. The existing quick-trap integration
fixture includes the native cases. `tools/test_whdload.py --setup a1200-040
--z3-memory --cache-memory --gameplay --ticks 0 --seconds 900` selects the
pinned cache-observer emulator via `FSUAE_WHDLOAD` and verifies the effective
NOMMU/32 MB Z3 configuration. Ordinary release builds retain no new probes.

MEASURED: all 384 native pointer cases and the complete quick-trap integration
route pass, including restoration and altered-evidence rejection. Host tests
also pass. The fixed release’s full 512×384 opening viewport matches the pinned Macintosh
frame under WHDLoad PRELOAD with high Zorro III allocations. The release also
passes the fresh-boot high-memory gameplay/quit gate: level 1, room 4 advances
eight updates, the original audio/window cleanup returns, all patches and host
vectors restore, and WHDLoad reports Return OK after Amiga-Q. The effective
68040/NOMMU, 32 MB Z3, zero Z2 and JIT-off configuration is checked, including
TC/CACR on return. This is correctness evidence, not a performance claim.
The earlier unrestricted-speed attempt reached the title but was interrupted
by its diagnostic WHDLoad TIMEOUT during loading; it is not a gameplay pass.
The accepted repeat retains the 040 preset timing and uses only a host deadline.
The new evidence
is pinned separately in `manifest/release-z3.sha256`. The initial short native
regression attempt timed out before the fixture; it is not a pass.

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
performance checks pass on a1200-030 for an earlier release. Later corrections
change MoveHHi error handling, display-buffer placement, the credits-page
services, heap placement and its cache flush, and resource publication; those measurements are not new measurements of this binary. See `timing.md`, `performance.md` and
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


## Display memory outside the application zone

WHDLoad report 7036 (A1230 68030/40, 32 MB) stopped with
`HEAP / HANDTOHAND ALLOCATION` at CODE 20+$085A during a return to attract
mode after rooftop deaths. MEASURED from the reporter's dump of the 0.90
executable: the 3 MiB zone had 428 KB free, but its largest block was 356 KB,
too small for the 374 KB copy that `LoadSaveDisRes` makes. The port had placed
its 640×480 screen buffer (307 KB) inside the application zone, whereas the
Macintosh screen lives outside the application partition. That buffer now has
its own allocation, as Vette! and Alone in the Dark do for their display
memory. It is still within the game's valid memory ranges and is released on
exit. MEASURED: in a release session on a1200-030 (`amiga/.run-EhBlxk`) the same
path completes the copy. The credits page it leads to then needed `GetPenState`
and `Move`; see "Credits page in the second attract pass" in
`docs/gameplay-coverage.md`. With both changes, the startup integration and
death/restart gates pass on the pinned FS-UAE.

## Heap placement and relocation

The owner's interactive WHDLoad session of the 0.91 candidate on a1200-030
(`tmp/whdload-play-p7o_4dek`) reached the credits after play and stopped at the
same CODE 20 `HandToHand`. MEASURED from its dump: the screen buffer was outside
the zone, which had 732 KB free, but the largest block was 370,544 bytes. A
master block and a 2,104-byte pointer sat between the two largest free blocks;
report 7036's dump has the same arrangement.

MEASURED on System 7.5.5 in a 64 KB private zone (`ref/mame/runs/masters-3`,
`tools/check_masters.py`, ten altered-record controls):

- `NewPtr`, `MoreMasters` and the implicit `MoreMasters` of `NewHandle` place
  their block at the lowest position above pinned blocks; the movable handle in
  the way moves, even across locked handles.
- A relocatable request that fits only by moving a handle across a locked one
  succeeds; a larger one returns `memFullErr`.

The native heap used first fit for master blocks and could move handles only
within the intervals between pinned blocks. It now does both of the above; the
relocation is used only after compaction and purging fail, so allocations that
already succeeded are unchanged. Relocation targets are first fit rather than
the original's rover. Host tests replay all ten original cases and fail with
either change removed.

MEASURED at the original's same `HandToHand` (`ref/mame/runs/credits-5`): a
3,033,356-byte zone with 284,988 bytes free and a largest block of 259,076, so
the original also makes room during the call. On the idle credits route the
native zone before the call has 373,824 bytes free in one block, 8 more than the
copy needs; with low placement alone the call failed
(`amiga/.run-wm5prO`), and with both changes it completes (`amiga/.run-fsZUUs`).
The release credits pairing passes on the corrected build (`amiga/.run-VOidea`).
The owner's exact session cannot be replayed: its input was not recorded.
`manifest/heap-placement.sha256` pins both original references and the
corrected build's credits pairing.

Low placement also moves handles that the old placement left alone, including
the unlocked 4-byte `Jnth` audio entry, which the original calls through its
handle. MEASURED: on 040/MMU with copyback caches the moved entry ran stale
bytes, and WHDLoad reported a Line 1111 exception just past it at startup
(`tmp/whdload-release-0bxqi9kg`). INFERRED: the original Memory Manager moves
blocks with `BlockMove`, which flushes the 68040 caches. Any heap move now
flushes the caches once before original code resumes; the 040/MMU save and
load cases pass with it.

## In-place resource publication

With the heap corrected, the owner's next WHDLoad session and idle WHDLoad runs
with 6, 6.5 and 7 MB of slave Fast RAM stopped at the credits with a privilege
violation at `$F80C38` in task 'DH0'. MEASURED from the dumps: exec still had
480 KB, 1.0 MB and 1.5 MB of Fast RAM free, so memory was not the cause. Each
run had just written the complete 550,792-byte `Prince2.opt.rsrc.aitd-new`.
On first use the original's `InitDissolve` adds its generated 373,788-byte
`DSLV 6401` dissolve cache to `Prince2.opt`; the port's publication then
renamed the files. WHDLoad's kickfs has no `ACTION_RENAME_OBJECT`, and without
DEBUG an unknown packet makes it jump past its action table into the Kickstart
reset code.

Under D20 the port never renames. Publication buffers the serialized fork and
writes it over the target with `MODE_NEWFILE`, as the original Resource
Manager rewrites a fork in place. MEASURED on the corrected build: the idle
WHDLoad route with the shipped 6 MB slave writes the fork in place without a
temporary file and runs 420 s without a report
(`tmp/whdload-idle-5yiuiv8k`); the native credits route enters, draws four
pages and completes the following attract cycle (`amiga/.run-NLyrfo`). The
two forks are byte-identical, with the same `DSLV 6401` payload as the
previous native publication. `manifest/in-place-publication.sha256` pins them.
The owner's interactive WHDLoad session of this build on a1200-030
(`tmp/whdload-play-8w1c2lif`) showed the credits pages without a crash.

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

`manifest/release-092.sha256` pins the 0.92 package and the results of these runs:

| Check | Completed run |
|---|---|
| stop-reason | `tmp/whdload-release-qm_ngegs` |
| key-right | `tmp/whdload-release-zxvxc9a6` |
| none | `tmp/whdload-release-j4ngvjbd` |
| joy-right | `tmp/whdload-release-vjkowc1c` |
| key-up | `tmp/whdload-release-iw5_j2n7` |
| joy-up | `tmp/whdload-release-kfinhkm0` |
| joy-second | `tmp/whdload-release-ccuyhhzq` |
| key-shift-right | `tmp/whdload-release-eslbkpp_` |
| key-ctrl-right | `tmp/whdload-release-dtffcnn7` |
| joy-fire-right | `tmp/whdload-release-scchdfkc` |

`dist/PrinceOfPersia2-0.92.lha`: **169,154 bytes**, SHA-256
`2250893429fbd21dac528a6f98b88201b65391a1fde40654c9a82a346e63113e`.

Release executable SHA-256:
`5b9963048ea682012e7d8953dc0930f5a5aa80c49e420a33c5113fecd389fd0a`.

### 0.91

`manifest/release-091.sha256` pins the 0.91 package and the results of the
runs below, plus the 040 crash report that the cache flush fixed. The integration and
restart gates passed on the same build (`tmp/integration-gate-oep8qy_y`,
`tmp/gameplay-restart-gate-w2_9nuy_`). `manifest/release14-current.sha256` and
`manifest/release14-evidence.sha256` retain their 0.90 and earlier scope.

| Check | Completed run |
|---|---|
| installer-fresh | `tmp/installer43-qd4jq_gg` |
| installer-reuse | `tmp/installer43-fjebdv04` |
| installer-replace | `tmp/installer43-q0hcwki7` |
| installer-incomplete | `tmp/installer43-uwn_lfl6` |
| installer-bad-options | `tmp/installer43-66qn50qp` |
| 030-preload | `tmp/whdload-release-3g1gl1eo` |
| 030-no-preload | `tmp/whdload-release-fu66qf9m` |
| 030-f10 | `tmp/whdload-release-hun_rat1` |
| a1200-030-save | `tmp/whdload-release-1lk7884m` |
| a1200-030-load | `tmp/whdload-release-kgghqnaa` |
| a1200-040-save | `tmp/whdload-release-i_qh30bo` |
| a1200-040-load | `tmp/whdload-release-zpyv_qak` |

The hidden damaged-preferences attempt `tmp/installer43-adu5zuos` was
interrupted after copying stopped progressing; its cause is unestablished and
it is not a pass. A fresh visible retry completed all checks. The harness now
supports `--window` for observation without changing its scripted answers.
The sandbox launch failure and the run interrupted for the owner’s date change
also remain non-passing attempts.

`dist/PrinceOfPersia2-0.91.lha`: **167,911 bytes**, SHA-256
`a32b0e34810de853fae9d8f0dc05aee35b2e320405d0a41d62c887d756def3f0`.

Release executable SHA-256:
`108e2f7eaf7fb5e116ec61b011cee2b224985bcab3044f4c880d05f1cd81acd0`.
