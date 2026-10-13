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
| Save/load | Save through session command, move, load, restored game state | File-routine fixtures only |
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
