# Gameplay coverage

Item 12 remains open. Completion requires positive original/Amiga pairs for
all entries below on `a1200-030`. Entry into a level does not prove its route
or completion. Service fixtures do not prove the corresponding session action.

The harness follows Vette's named route/checkpoint approach and Alone in the
Dark's ordinary-input session routes. Any supplied checkpoint must be identified
separately from uninterrupted play. Diagnostic input and observations are excluded
from ordinary release builds.

## Current evidence

The original restart route starts a game with Space and sends Command-R after
60 reference input frames. It observes the original `HandleMenuChoice` command
`01930009` and two completed `FirstFrame` returns in level 1, room 4. The
original `RestartGame` routine remains active across the restart. It does not
need to be called a second time.

MEASURED: `gameplay-restart-1` and `gameplay-restart-2` have identical complete
A5/context captures, screen indices and RGB palette at both checkpoints.
`manifest/gameplay-restart-reference.sha256` pins names and hashes only. The
second run additionally records the original menu-handler command. This is
paired with completed native runs: screen indices, all eight bitplanes, palette,
curated gameplay words and RNG match at both checkpoints. The final Copper list
also matches the original palette and required display mode. Two native runs have
identical pixels, bitplanes, colour tables, curated state and RNG. Hardware and
heap restoration pass; all 15 evidence-corruption controls reject.

`make gameplay-restart-check` reproduces two original and two native routes,
checks completion and rejection controls, then restores the ordinary release
build. This gate covers restart only.

Reproduce just the original route in the shared environment:

```sh
POP2_GAMEPLAY=restart POP2_DISPLAY_ROUTE=1 python3 tools/run_reference.py
```

The opening trace is retained through its audited checkpoint. The gameplay
observer then disables the broad trap trace and records named lifecycle events
and bounded captures. Original bytes, saved state and captures remain ignored.

## Acceptance matrix

| Entry | Required positive endpoint | Paired coverage |
|---|---|---|
| Level 1 | Meaningful gameplay route and original level-exit decision | Original fresh-run repeats pass; native pair pending |
| Level 2 | Meaningful gameplay route and original level-exit decision | Original fresh-run repeats pass; native pair pending |
| Levels 3–14, individually | Meaningful gameplay route and original level-exit decision | Incomplete |
| Main cutscenes, individually | Original natural return and paired visible/state checkpoints | Incomplete |
| Timed story scenes, individually | Original natural return for each selected scene | Incomplete |
| Save/load | Save through session command, move, load, restored game state | Two original/native pairs; 22 rejection controls |
| Death | Original death routine returns after gameplay input | Existing first-room integration route |
| Restart | Original command followed by completed playable first frame | Original and native repeats pass; 15 rejection controls |
| Hall of Fame | Show/dismiss and persistent score insertion/readback | File-routine fixtures only |
| Ending | Final gameplay decision, ending return and subsequent session flow | Incomplete |

DERIVED: CODE 2 `RestartGame` accepts levels 1–14; the session loop at
`CODE 2+45A4` selects the ending path for a result above 14. CODE 15's NIS
selector at `+0088` has separate main-cinematic and numbered story-scene paths;
these must be enumerated by their original dispatch and callers before claiming
complete cutscene coverage. These static facts establish scope, not execution.

## Walkthrough reference

The owner-supplied [walkthrough](https://gamefaqs.gamespot.com/pc/564689-prince-of-persia-2-the-shadow-and-the-flame/faqs/23644)
is for the original 1993 Prince of Persia II: The Shadow and the Flame, IBM version.
It is a route-planning aid. Its PC level numbering and platform-specific details
must be checked against the Macintosh original before use as validation evidence.

## Save/load reference

MEASURED: the original session saves `Level 1` beside the application, moves the
Prince, selects that document through Open, and resumes gameplay. Loading reaches
copy protection; this fixture supplies the D6-authorized accepted condition, as
in the existing UI fixture. Both a new first frame and the following gameplay
frame complete. An explicit original `PBFlushVolSync` then returns successfully.
The save is a level checkpoint, not an exact-position snapshot.

Two independent original runs reproduce the same saved 100-byte data fork, empty
resource fork, type/creator, captured game state and pixels. The raw MacBinary
wrapper is preserved privately; its filesystem timestamps vary between runs.
`manifest/gameplay-save-reference.sha256` binds the stable evidence.

Two native runs match the saved file, RNG, 62-byte player record, selected level
state, three completed frame images, AGA bitplanes and palettes. Hardware/heap
restoration passes. All 22 evidence-corruption controls reject. The diagnostic
harness replays the original consumed keys by game step: Save at 12, Left on steps 14–17, and
Open at 24, with the measured Command-key transitions. This replay uses the
existing native input queue and is excluded from release builds.

`make gameplay-save-check` creates two original references, exports their flushed
saves, repeats the native route, runs rejection controls and restores the release
build. All 26 consumed key-state records must match. The 62-byte player record
is identified by the original record copy at CODE 4+3272; no pointer or
clock masks are applied to it. Save/load is a session route, not a whole-level
route. This gate does not yet prove reading this application-folder save after a
fresh boot; the earlier persistence fixtures use a file in `Data`.

## Original route discovery

`POP2_GAMEPLAY=route POP2_DISPLAY_ROUTE=1 python3 tools/run_reference.py --name NAME`
starts an interactive, diagnostic Macintosh route capture. In its ignored run
folder, `route-status.txt` identifies the last completed original frame and
`route-N` files retain the original state, indexed screen and palette. Supply
`route-command.txt` as one line containing the next sequence number, completed
frame count (1–600), and comma-separated keys, for example `1 30 left`.
Use an atomic rename when replacing the command file. `none` releases all keys.
Available keys include arrows, Shift, Control, Space, Return, Escape, Command and R.
Control selects the original sword action; Shift is the distinct careful-movement
and grabbing action. While armed, Shift plus the backward direction turns the
Prince without sheathing (CODE 6+22F2 input branch; DERIVED and confirmed by
original captures in both directions). The controller uses ordinary ADB input and logs the actual
KeyMap consumed by the original, rather than assuming each requested key was read.

The headless debugger resumes after its callback returns. While awaiting a host
command, this observer therefore stays inside the completed-instruction callback;
no emulated time passes during inspection. A 15-minute host input deadline is a
failure, not a completed route. Commands and captures are discovery evidence only.
Death can end an interval early and a new first frame is identified explicitly.
Only a completed first frame in level 2 emits the level-1 completion marker.
Original copy protection uses the existing D6 accepted-branch fixture.

MEASURED: discovery captures have advanced by requested intervals of 35 and 24
completed steps, remained at the same boundary between commands, and observed
ordinary room transitions and death/restart. These observations validate the
controller's basic operation; they do not establish a completed level route or
an original/Amiga pair. Input schedules still need fresh-run repeatability and
native replay before they can become acceptance evidence. Two discovery runs
reproduce identical full initial A5/context, screen and palette captures and the
same first 30 consumed keys: the initial read is unpressed, followed by 29 Left
reads. This observed ADB delay must be preserved in a native replay.

### Discovery save states

At a paused gameplay boundary, `N save SLOT` saves an emulator checkpoint and
`N load SLOT` restores it; `N` is the next command sequence number. Slots use
letters, digits and hyphens, up to 48 characters. Existing slots are never
overwritten. States stay in the ignored run folder under `states/maciici/`.
Loading requires a slot created in the same running controller session, outside
an active cutscene. Sequence numbers continue increasing after a restore, so
captures and command history are retained.

The controller uses immediate debugger save/load operations at the stopped CPU
boundary, verifies restored CPU registers and full A5/context bytes, and restores
its frame counters and held keys. This is a discovery shortcut for movement and
combat retries. Do not cross game-save or other disk writes when restoring:
emulator state does not roll back the disk image. Ordinary game saves remain
level checkpoints. Successful routes still require fresh fixed-seed original
replay and paired native execution for acceptance.

MEASURED: `gameplay-checkpoints-1` saved the initial room, ran 30 completed Left
steps, restored the initial checkpoint, and repeated those steps. Saving and
loading preserved identical full A5/context, indexed screen and palette captures;
the repeated continuation also matched all four captures exactly. The original
save and load checks additionally matched CPU registers. Separately,
`gameplay-level1-discovery-4` and `gameplay-level1-discovery-5` reproduced all 45
checkpoints through the rooftop approach with identical A5/context, screen and
palette. Neither result establishes level-1 completion.

A longer restored continuation in `gameplay-checkpoints-1` matched the original
full A5 captures through approach checkpoint 28, then differed at A5−0x7A7 in
checkpoint 29. That byte belongs to the timer longword read and cleared by
CODE 21 `osGetTime` (0x6A68/0x6A70; DERIVED). Context, indexed screen and palette
continued to match through approach checkpoint 45. Thus the 30-step restore test
is bounded evidence, not a claim of indefinite full-memory replay equivalence.

## Level 1 original completion

MEASURED: `gameplay-level1-complete-1` and `gameplay-level1-complete-2` each
execute a fresh fixed-seed route from the first room through combat, rooftop
jumps, the raised-roof climb, controlled descent, docks and ship grab. Each
observes one entry and natural return of `PlayNIS` selector 9, then the completed
first frame of level 2, room 2. Both emulators exit normally and their reference
launchers pass. No emulator restores or gameplay-state edits occur in these
fresh runs; the existing owner-authorized D6 protection seam remains available.

The authored schedule in `tools/gameplay_routes/level1.tsv` contains 95 keyboard
actions. The initial checkpoint and all 95 action endpoints match between the
two fresh runs: full 0x2200-byte context, six 62-byte actor records, RNG,
level/room/step counters, indexed screen and palette. The final first frame is
at completed gameplay step 1001. All 1002 consumed KeyMap records also match.
Full A5 dumps are retained and pinned for the
canonical first reference, but full-A5 repeatability is not claimed because
platform timer bookkeeping can differ. These runs prove original level-1
completion and repeatability, not an Amiga pair or cutscene pixel coverage.

`manifest/gameplay-level1-reference.sha256` pins the canonical first run's
captures, input actions and completion log by name and hash only. Run the
original controller as above with a fresh name, then feed it from another shell:

```sh
python3 tools/replay_gameplay_route.py ref/mame/runs/NAME \
  --reference ref/mame/runs/gameplay-level1-complete-1
```

The replay verifies canonical reference pins before comparison, uses ordinary
ADB keys, requires every intermediate completed-step interval, and rejects an
unreached next-level endpoint. The reference launcher separately requires normal
exit and a unique positive completion marker. `python3 tools/test_gameplay_route.py`
checks 14 evidence-corruption/extent/pin rejections, six malformed schedules and
an unreached endpoint, with passing controls before corruption checks. The
completed reference disk images are compressed with SHA-256 round-trip checks;
all original captures and emulator state remain ignored.

## Continuing discovery sessions

`POP2_ROUTE_DISCOVERY=1` keeps the original route controller open across level
boundaries. Its separate `N finish` command terminates a discovery session with
`GAMEPLAY_DISCOVERY_COMPLETE`; this marker is not a level-completion gate.
Existing `N save SLOT` and `N load SLOT` commands preserve the current level in
the controller as well as the captured CPU/game state. The ordinary level-one
reference mode still exits only after its original next-level decision.

MEASURED: `gameplay-level2-discovery-1` reproduces all 96 pinned level-one
checkpoints before saving the level-two shipwreck entry. Restoring that save
reproduces full captured A5/context, screen indices and palette exactly; two
30-step continuations also match all four captures. This checkpoint supports
beach and cave-entrance retries; level-two original completion is now repeated
as described below. Its native pair remains unverified.

MEASURED: the continuing discovery run completes the level-two stone puzzle
and reaches level 3, room 1, at game step 1360 (`route-144`). It uses only
original gameplay inputs and the existing D6 protection seam; checkpoint
restores support retries. `tools/gameplay_routes/level2.tsv` records the
successful 19-action path. The level-three first frame is saved as
`level3-start` in that still-open session. This is discovery evidence, separate
from the fresh repeats below. The native pair remains required for level-two
acceptance.

## Level 2 original completion

MEASURED: `gameplay-level2-fresh-1` and `gameplay-level2-fresh-2` each replay
level one from a fresh boot, then the 19 authored level-two actions, without
emulator saves or restores. Both reach the original level-three first frame
at step 1360, room 1, and exit normally through the discovery finish command.
All 96 level-one checkpoints still match the pinned reference. The 20 level-two
entry/action checkpoints match between fresh runs: complete 0x2200-byte context,
six actor records, RNG, level/room/step counters, screen indices and palette.
All 1362 consumed key maps match. Step 1001 is read twice across the level
boundary; the input evidence preserves both reads.

`manifest/gameplay-level2-reference.sha256` pins the first fresh run's captures,
actions and completion log. Full A5 captures are retained, but only the stated
fields are compared for repeatability. Reproduce the original route with a
fresh discovery run, then feed it from another shell:

```sh
POP2_GAMEPLAY=route POP2_DISPLAY_ROUTE=1 POP2_ROUTE_DISCOVERY=1 \
  python3 tools/run_reference.py --name NAME
python3 tools/replay_gameplay_route.py ref/mame/runs/NAME --through-level 2 \
  --reference ref/mame/runs/gameplay-level2-fresh-1
```

`tools/check_gameplay_level2_reference.py FIRST SECOND` requires both fresh
routes, their authored actions, completed frame intervals, actual input reads,
natural level transitions and paired state/pixels. The companion
`tools/test_gameplay_level2_reference.py FIRST SECOND` passes 17 rejection
controls for completion, restores, transitions, input, capture state, pixels,
truncated evidence and reuse of a single run. The route-replayer unit tests
also reject missing level transitions and an unreached finish. Completed disk
images are compressed with verified round trips. Saves and captures stay ignored.
This proves original level-two repeatability, not its Amiga pair.

### Macintosh save import

MEASURED: `gameplay-import-original-2` and `gameplay-import-original-3` load the
pinned original-created `Level 1` into fresh Macintosh sessions without saving.
They reproduce three full A5/context captures, indexed screens, palettes and
all 25 consumed keyboard records. Open is handled at step 23; ResumeGame, the
loaded first frame, the following gameplay frame and the volume flush complete.
The added document changes Finder startup, so the repeated opening image and
exact trap trace have their own `manifest/reference-import-startup.sha256`.
Frame hashes are in `manifest/gameplay-import-frames.sha256`; the original save
wrapper is pinned in `manifest/gameplay-import-reference.sha256`.

Fresh native runs `.run-1gPmFM` and `.run-fBAvX7` match those original checkpoints:
RNG, player record, selected context, pixels, AGA planes, palette, Copper output
and consumed keys. Neither native session saves before loading. The original
100-byte data fork, empty resource fork and imported Finder metadata remain
unchanged. Hardware, heap, original-code and input restoration pass. The two
native runs repeat the measured state and presentation. Fourteen original and
19 native evidence-corruption controls reject altered completion, input, files,
build identity, game state, rendering and restoration. The combined result is
`GAMEPLAY LOAD-IMPORT GATE PASS`; ordinary release restoration also passes.

`make gameplay-import-check` repeats this gate using the pinned original save
export in the ignored `gameplay-save-load-keys-1` reference folder. The existing
`tools/check_gameplay.py --route load-import --save-source PATH` option selects
an equivalent pinned export. Saves restore a level checkpoint, not an exact
position. Import staging and scripted inputs require an explicit verification
build; ordinary release startup does not scan save hashes.

### Level-three original repeatability

MEASURED: `gameplay-level3-fresh-1` and `gameplay-level3-fresh-2` execute the
authored route from fresh Macintosh startup without emulator restores. All 181
gameplay checkpoints match in full context, six actor records, RNG, counters,
indexed pixels and palette. Both consume the same 2770 keyboard records and
reach level 4, room 1, at step 2767. Story selectors 7, 4, 9, 100 and 10 each
have ordered entry and return records. This establishes scene completion on
the original, not paired cutscene rendering on Amiga.

`tools/check_gameplay_level2_reference.py --level 3 FIRST SECOND` checks the
complete route, natural transitions and scene completion. Its companion
rejection test accepts the same option and rejects 25 corruptions or invalid
run pairs, including a missing scene return and altered late-level evidence.
`manifest/gameplay-level3-reference.sha256` pins the first run's captures,
actions and log. `tools/replay_gameplay_route.py --through-level 3` replays
the authored route for further original repetitions. Native level-three
validation remains open; the earlier level-one RNG mismatches remain failures.

### Level-four original repeatability

MEASURED: `gameplay-level4-fresh-1` and `gameplay-level4-fresh-2` replay all
265 authored input intervals from fresh Macintosh startup without emulator
restores. All 266 gameplay checkpoints match full context, six actor records,
RNG, counters, indexed pixels and palette. Both consume 4660 matching keyboard
records and naturally enter level 5, room 1, at step 4656. Story selectors 7,
4, 9, 100, 10 and 20 have ordered entry and return records. These records prove
original scene completion; native pairing and cutscene rendering remain open.

`tools/gameplay_routes/level4.tsv` contains the 85 level-four input intervals.
`tools/check_gameplay_level2_reference.py --level 4 FIRST SECOND` checks the
complete route and transitions. The companion rejection test rejects 33
corruptions or invalid pairs, including altered late-level state, pixels,
palette, RNG, keyboard order and missing scene completion.
`manifest/gameplay-level4-reference.sha256` pins 1070 capture, action and log
files from the first fresh run. `tools/replay_gameplay_route.py --through-level 4`
replays the complete authored route. Item 12 remains open.

### Level-five discovery status

MEASURED: the discovery session crosses the entrance crouching without losing
health, defeats the first skeleton (`route-823`, step 4943, enemy health zero),
and crosses the lava with all three health points (`route-829`, step 5007,
room 2). `level5-skeleton-dead-full-health` and
`level5-trap-entry-full-health` preserve these checkpoints.

The room-two crossing is not complete. Captures `route-850`, `route-871`,
`route-908`, `route-961` and `route-966` distinguish the upper closing plate
from the lower switch and the second door's changing state. DERIVED: walking
onto the upper plate closes the first door; the lower switch gives limited
time to pass the second. Waiting at the first door and varying the jump delay
there did not provide a crossing. The earlier suggestion that both doors
could simply be crossed after crouching along the upper platform is not
supported by these attempts.

MEASURED: the shorter lower return (20 frames left/down, 16 right, 30 up/shift)
reaches the upper platform with full health (`route-1104`, step 5073).
`level5-upper-329-full-health` preserves a standing approach at x 329, step
5092. Four frames right followed by 14 right/up reaches x 601, y 226 with
full health (`route-1163`), but continuing right falls into the pit and dies
(`route-1164`). The similar low landing in `level5-right-pit-full-health`
also fails to climb out before the door blocks it. Neither is a successful
crossing. The next route work needs an earlier second-door arrival while
avoiding the upper closing plate. No level-five completion, repeatability or
native comparison is claimed.

Restored discovery attempts have sometimes diverged after identical short
commands when the preceding held input differed (for example `route-853`
versus `route-856`). Releasing inputs before restore helps exploration;
the input-continuity investigation is deferred in item 17. Discovery states
are navigation aids, and only fresh repeat runs can establish route acceptance.
