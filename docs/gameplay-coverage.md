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
| Levels 1–14, individually | Meaningful gameplay route and original level-exit decision | Incomplete |
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
and grabbing action. The controller uses ordinary ADB input and logs the actual
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
