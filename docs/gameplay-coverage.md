# Gameplay coverage

Item 12 is accepted on `a1200-030` under owner decisions D10–D12. Recorded
original/native routes reach every level exit, all cinematic cases, session
save/load, death, restart, Hall persistence/readback and the ending. Assistance
is explicitly logged and replayed. This is coverage by multiple reproducible
routes, not a claim of one uninterrupted unassisted playthrough.

D12 accepts the recorded one- or two-frame exit differences and remaining
level-3/4/13 animation, RNG and non-player actor-slot differences for this item;
Item 16 is complete under D18: the recorded differences have original-paired
causes and gameplay-effect evidence. They are accepted timing consequences,
not exact frame/state matches. Service guards remain active.
The strict diagnostic reports deliberately remain failed (`route_accepted:
false` where present). Their captured next-level entries are positive exit
evidence; an entry obtained solely by a level cheat is not exit evidence.
No unresolved service stop remains in the accepted routes. Earlier failing
runs below are diagnostic controls, not accepted substitutes for later endpoints.
Unknown services and new differences remain failures. Release/040, 68020 and
real-hardware validation are not established by this acceptance.

The harness follows Vette's named route/checkpoint approach and Alone in the
Dark's ordinary-input session routes. Any supplied checkpoint must be identified
separately from uninterrupted play. Diagnostic input and observations are excluded
from ordinary release builds.

The recorded `context.bin` captures cover exactly the first 0x2200 bytes of the
level-state allocation whose pointer is stored at A5−5014. CODE 6+3642 locates persistent
opponent records in a separate per-room table: 32 records of 192 bytes starting
at context+0x21A6, each containing a count and five 38-byte opponent records.
Most of this table falls outside the older captures. In particular, CODE 4's
skeleton decisions read words +28/+30/+34 from these persistent records for
timing and shaking state. Equality of the six active actor records and the 0x2200-byte context
therefore does not establish equality of all AI state. Item 16's opt-in
`POP2_ROUTE_OPPONENT_STATE` / `--opponents` observations capture that bounded
table separately; the paired timer fixture below establishes its role in the
late actor differences.

## Closure audit

MEASURED: the closure audit rechecked 22 cinematic, route and session checks
against the retained evidence. All pass within their documented D11 scope.
This includes all 20 cinematic selectors, levels 1/2/5/9/14, save/load,
restart, Hall insertion/readback and the opening/death route. The death run
`.run-4ymwYZ` records stage 4, kind 3, followed by route completion and
restoration; its opening comparison uses `gameplay-natural-opening-captures-3`.
The original audio route `integration-natural-title-1` independently records
kind-3 death followed by continued death audio and observer completion.

The nine D12 exceptions below have verified native build identities,
code/input audits and restoration. All 3,097 files in their native manifests
were rehashed. The six counter-only endpoint comparisons were rerun and retain
`route_accepted: false`; D12 accepts coverage without changing those results.
`manifest/gameplay-closure.sha256` pins the ignored audit indexes, containing
check names, run identities and results. Level-four native evidence is now
pinned by `manifest/gameplay-level4-native.sha256`. The ordinary release link
audit also passes; no runtime code or diagnostic defaults changed for closure.

| Level | Native run | Original run | Native manifest (under `manifest/`) |
|---|---|---|---|
| 3 | `.run-Vz4heJ` | `gameplay-level3-fresh-1` | `gameplay-level3-rejected-entry.sha256` |
| 4 | `.run-0c0hOQ` | `gameplay-assisted-level4-1` | `gameplay-level4-native.sha256` |
| 6 | `.run-7yMRHs` | `gameplay-level6-door-1` | `gameplay-level6-rejected-entry.sha256` |
| 7 | `.run-vyUFKZ` | `gameplay-assisted-level7-clean-1` | `gameplay-level7-rejected-entry.sha256` |
| 8 | `.run-EhJVft` | `gameplay-level8-direct-stairs-1` | `gameplay-level8-direct-stairs-native.sha256` |
| 10 | `.run-CeIpbt` | `gameplay-assisted-level10-clean-1` | `gameplay-level10-rejected-entry.sha256` |
| 11 | `.run-sm9wiN` | `gameplay-assisted-level11-1` | `gameplay-level11-rejected-entry.sha256` |
| 12 | `.run-XYaGwq` | `gameplay-assisted-level12-1` | `gameplay-level12-rejected-entry.sha256` |
| 13 | `.run-jXfO5v` | `gameplay-assisted-level13-flame-1` | `gameplay-level13-rejected-entry.sha256` |

The acceptance matrix above governs current status. Detailed failed discovery
controls and intermediate comparison findings below remain retained evidence;
they do not reopen item 12 or establish exact equivalence.

## Timing and state differences resolved under D18

Item 16 is closed by verified explanation under D18. Ambient-sound scheduling
shifts consumption of the shared RNG stream; original counterfactuals establish
the effects on torch animation, actor decisions and persistent skeleton timers.
The death-sound gate shifts shadow creation and the altar flame reset by one
frame. Exit traces identify sound-completion branches as the source of the
recorded frame-count differences. Additional draws around the level-eight and
level-thirteen transitions account for their terminal RNG differences.
No production RNG, actor state or clock policy was changed to force equality.
The first-room parameter-block residue comparison is the scoped correction
described in `docs/native-integration.md`.

| Exit level | Original step | Native step | Paired evidence scope |
|---|---:|---:|---|
| 1 | 1001 | 1000 | Current probes; entry pixels, context, actors and RNG match |
| 3 | 2767 | 2766 | Replay of the pinned historical level-three diagnostic |
| 4 | 1889 | 1888 | Existing timer/actor diagnostic and fresh original exit trace |
| 6 | 333 | 331 | Exit, queued-note timestamp and delivery observers |
| 7 | 359 | 356 | Fresh exit audit; older native capture exited at 357 |
| 8 | 1193 | 1190 | Fresh exit audit and paired boundary RNG calls |
| 10–12 | 292 | 290 | Separate fresh exit audits for all three levels |
| 13 | 1517 | 1515 | Exit/RNG audit tied to the shadow/flame causal pair |

These are game-step observations on the retained emulator, not calibrated
performance claims. Current full-route probes stop at the earlier level-two
timing guard; they do not establish a current full level-three replay. The
historical replay below resolves the recorded level-three difference. The
strict guards and failed comparisons remain intact. Corrected-clock and release
qualification remain items 22 and 14 respectively.

The first-room reference A5−0B77 difference is resolved as retained volume-date
metadata; see `docs/native-integration.md` for original call/return provenance,
the narrow comparison change and controls. This does not resolve the gameplay
differences below.

MEASURED: `tools/check_gameplay_effect_lifetimes.py` pairs all 144 natural effect
completions after gameplay entry in `gameplay-level1-audio-3` with the current
production `HalestormEngine::effectOn/advance`. All complete after exactly the
same number of synthesis blocks. The host probe passes ASan/UBSan and two
missing-evidence controls. `manifest/gameplay-effect-lifetimes.sha256` pins the
original trace and driver. Explicit stops/replacements are excluded by their
distinct completion caller; the measured natural mixer return is driver+2C7C.
This establishes logical effect lifetimes for these descriptors, not wall-clock
phase or when gameplay observes completion.

DERIVED: CODE 2+6134 tests a pending level change, calls CODE 5+4E76 and
`PrinceSndOn` (CODE 5+52C8), then either enters the next level through +61CE or
executes another `FrameAdv` at +616A. Sound completion can therefore change
the exit frame count.

MEASURED: original `item16-exit-level6-2` repeats all checkpoints of the pinned
`gameplay-level6-door-1` route. Read-only observers on original CODE 2+6134,
+6144, +6152 and +61CE show a pending transition from level six to seven at
frame 122 in both versions. The ambient sound test returns nonzero until frame
333 on the Mac and frame 331 in native `.run-b9r2xF`; both then pass the second
sound test and take the original transition branch. All 24 preceding native
checkpoints and the rejected next-level entry match pixels, palette, context
and actors; endpoint RNG also matches. `tools/check_gameplay_exit_gate.py`
checks the complete consecutive branch observations, original repeat,
native endpoint and three evidence-corruption controls, using
`manifest/gameplay-item16-exit-gate.sha256`. This explains the immediate branch
responsible for the differing frame count, not the underlying audio timing.
The strict native route remains failed. The earlier original observer run
`item16-exit-level6-1` used unsupported decimal log formatting and is not valid
exit-trace evidence; the corrected observer uses hexadecimal fields.

The additional original `item16-exit-level6-audio-1` repeats every canonical
checkpoint with the audio observer enabled. Its last effect completion lies
between the pending-transition observations at frames 123 and 124, while the
ambient gate remains closed until frame 333; the ambient
record has the MIDI type. `manifest/gameplay-item16-exit-audio.sha256` pins this
reference. The native in-game audio trace `.run-1QQeIk` was stopped during slow
startup without a checkpoint. A read-only observer on the byte-identical
baseline executable, `.run-meBaoD`, also reached its 900-second host deadline
before a checkpoint. Neither is passing route or audio timing evidence, and
the slowdown cannot be attributed to the in-game trace from these observations.

MEASURED: the longer read-only native run `.run-eNDhoJ` completes observation
and restoration, reproducing the rejected entry at frame 331. Its executable is
byte-identical to `.run-b9r2xF`. The pending-exit music contains the same 194
note requests, with three adjacent pairs issued in a different order.
Between the bounding stop requests, the most recent driver-clock queries span
1124 ticks on the Mac and 1116 natively. These are query observations, not
exact timestamps for the stop requests. `tools/check_gameplay_music_window.py`
checks the pinned requests, original call/return pairing, exit branches, complete
native endpoint comparison and two missing-evidence controls. The extended
`manifest/gameplay-item16-exit-audio.sha256` pins the native observation too.
This narrows the discrepancy to music scheduling; it does not establish its
underlying clock cause or accept the failed route.

MEASURED in that same complete 194-note window: the original returns 1,129
clock queries with increments 0/2/3 occurring 722/94/312 times. Native returns
1,116 queries with increments 0/1/2 occurring 185/744/186 times. The checker
requires monotonic queries, bounds the window by the same stop requests, and
rejects backwards-clock corruption on both sides. Thus driver-clock batching
differs even though the note multiset agrees. DERIVED: the original expanded
driver increments its clock in the Sound Manager buffer callback at +082A
(and in its alternate output routine at +0950); native `NativeAudio::field`
accumulates six ticks per five PAL fields. This identifies a scheduling
mechanism to test, not a causal proof for the exit offset or authorization to
change the release clock.


The initial clock-replay attempts `item16-music-clock-1`,
`item16-music-clock-control-1` and `item16-music-clock-2` are failed controls:
all stop at checkpoint 7 before any clock substitution. Their second recorded
key read is empty, whereas the canonical input has Left pressed; the RNG seed
still matches, but context and pixels do not. Identical host commands were
insufficient to reproduce that input read. These captures cannot establish a
clock-replay effect. `manifest/gameplay-item16-clock-prefix-control.sha256`
preserves them; the revised fixture pins and replays the reference key reads.
`item16-music-clock-control-2` then reproduces all 16 canonical checkpoints
and the complete original music schedule using the pinned key reads. Its clock
trigger is unreached, so it is an activation-failure control, not a passing
clock fixture. The trigger had used the preceding key-read frame rather than
the counter after `FrameAdv` increments it. The trigger-control manifest and
checker retain both its passing input replay and failed activation. The revised unchanged-value control `item16-music-clock-control-3` positively
activates at step 109, observes 1,119 queries, and preserves all 16 checkpoints
and the complete original note schedule. The substitution run
`item16-music-clock-3` completes but fails the clock-fixture query-count gate:
it consumes the 1,115 pinned native responses and then four extrapolated
formula responses. Those four are outside the pinned native stream, so this
is a failed counterfactual, not proof of an equivalent native schedule.
It still exits at step 333. Clock-query batching alone has not been established
as the timing cause. The clock-control and clock-replay manifests retain both
runs, and the strict replay checker continues to reject the substitution.
Future substitutions stop at the end of the pinned response stream; that new
runtime exhaustion guard has not yet been exercised in the emulator.

DERIVED: `HandleLiveMusicVBLTask` (CODE 11+0004) compares queued-note timestamps
with `GetDriverSyncTime`. The note-on enqueue path at +0266 instead uses
`TickCount + 1 - A5[-11D4]`; +019E establishes that TickCount origin and resets
the driver clock. Replaying only the driver's increments therefore does not
replay both scheduling inputs. MEASURED: in the pinned original 194-note
window, the origin is 0xC8A and `TickCount - origin - driver clock` at the
note-on calls ranges from -1 to 2 (28/93/69/4 calls respectively).
`check_gameplay_music_window.py` checks those observations and rejects a
corrupted origin. These are delivery-call observations, not enqueue timestamps.
The native trace lacks corresponding TickCount samples, so the relative phase
has not yet been paired and is not an established cause of the exit offset.

MEASURED: native `.run-YvbwCb` adds read-only queued-timestamp observations
to the same diagnostic executable as `.run-eNDhoJ`. It preserves all 24
preceding checkpoints, the complete matching entry state and the rejected
331-versus-333 exit. Both versions deliver the same 194 note-on packets and
194 note-off packets. The note-on enqueue spans are 960 original / 956 native
ticks; note-off spans are 1084 / 1079. Original delivery is zero to two ticks
after each queued deadline; native delivery is zero to one. Relative to the
last clock query before the initial stop, the final note-off is due at
1114 / 1105 and delivered at 1116 / 1105. Both subsequent stop requests
observe a clock query eight ticks beyond that final delivery. Thus different
timestamps already exist before delivery; changing only driver-clock queries
cannot reproduce the complete scheduling input. The stop-query spans in this
run are 1124 / 1113; the earlier native run observed 1116, while both enter at
frame 331. These are scoped query observations, not calibrated wall-clock
performance measurements. `check_gameplay_note_timestamps.py` checks both
request streams, deadlines, original/native exit gates, preceding/entry state
and missing/corrupt-timestamp controls. The new native manifest pins the run.
The first attempt `.run-QvZ6AU` failed when sandbox policy blocked the debugger
listener; it contains no gameplay checkpoint and is not comparison evidence.

MEASURED: level-seven audit `item16-exit-audit-level7-1` repeats the full
canonical route and consumed inputs. Native `.run-8h2qSE` starts the pending
transition at the same step 147, but the sound test releases it at 356 versus
359 on the original. The older native capture released at 357. All 23 preceding
checkpoints and the next-level entry match original pixels, palette, captured
context, actors and RNG; the entry step/key counts remain different. The exit
audit reuses the strict rejected-entry inspector, checks every consecutive
sound branch and rejects removed transitions. The two
`manifest/gameplay-item16-exit-audit-7-*.sha256` manifests pin this pair.
This is observed sound-gate timing variation, not a fixed two-frame offset or
a passing strict timing comparison.

MEASURED: the following original exit-audit repeats match their complete
canonical routes and consumed inputs. Each native run matches all 23 preceding
checkpoints and the next-level entry's pixels, palette, captured context,
actors and RNG. Both sides begin waiting at step 138; the first sound test
releases at 292 on the original and 290 on native. The strict timing failure
is retained, and neither native run changes the older recorded native state.

| Level | Original run | Native run |
|---|---|---|
| 10 | `item16-exit-audit-level10-1` | `.run-kxs2QD` |
| 11 | `item16-exit-audit-level11-1` | `.run-zEfQT3` |
| 12 | `item16-exit-audit-level12-1` | `.run-vhFoUL` |

The corresponding `gameplay-item16-exit-audit-LEVEL-original.sha256` and
`-native.sha256` manifests pin the observations; `check_gameplay_exit_audit.py`
checks the complete sound branches, strict rejected-entry state comparison,
build/restoration audits and missing-transition controls for each pair.

MEASURED: corrected level-eight repeat `item16-exit-audit-level8-2` matches
its complete canonical route. Native `.run-a9ls3Q` matches all 77 preceding
checkpoints and the next-level entry's pixels, palette, context and actors,
but its entry RNG differs from the original. The pending exit
begins at 981 on both; the sound gate releases at native 1190 versus original
1193. The older native capture released at 1191 and matched the original entry
RNG. The default strict rejected-entry comparison therefore fails on RNG.
The exit-audit-8 original/native manifests pin the runs.

MEASURED: bounded original `item16-exit-rng8-1` repeats the complete route.
At step 1193, `PlayAmbient` (CODE 5, return +50BA) draws once while
still in level eight, before the exit branch, then again after entering
level nine. Native `.run-9Y2tkt` reproduces `.run-a9ls3Q` in every captured
state, colour table and plane buffer; its only draw in the observed
982–1200 window occurs at step 1190, after entering level nine, and returns
the value consumed by the original pre-transition call. Both versions have
the same seed at the preceding step-982 capture.
The extra original pre-transition ambient call therefore accounts for the
terminal seed difference; neither RNG results nor actor records are normalized.
`check_gameplay_exit_rng8.py` verifies the caller/transition order, captured
seeds, complete original repeat, unchanged native state, restoration and two
corruption controls. The two `gameplay-item16-exit-rng8-*.sha256` manifests pin
that proof. The exit audit accepts this verified timing consequence under D18
while reporting the unequal RNG and failed strict timing explicitly. The
standalone rejected-entry inspector retains strict comparison by default.

The first level-eight repeat used asynchronous keyboard input instead of the
reference's explicit direct-key mode: its first Down read arrived at step 22
rather than 21. It correctly failed checkpoint 9 and supplies no accepted
exit evidence. The initial frozen native audit also stopped before gameplay:
its older executable expected the previous relocation table and rejected the
current inputs with `CORE INPUT / SHA256`. The failed-controls manifest pins
both failures; subsequent audits use current, input-matched diagnostics.

MEASURED: the final-level exit audit pairs original
`item16-exit-audit-level13-1` with native `.run-64CRkE`. Both begin the pending
level-fourteen transition at step 1363; the original sound-completion branch
releases it at 1517, the native branch at 1515. The original repeats every
canonical checkpoint and consumed key map. All native actor, RNG, step, screen
and captured-context states reproduce the already paired shadow-gate run
`.run-NeBYwx`; the differences from the older item-12 native capture remain
reported separately. Clean restoration and code/input audits pass.

The bounded final RNG trace matches the first 52 draws, including frame,
seed/result and mapped original caller. During the two additional original
waiting frames, `TempGetTorchFrame` (CODE 10+12B6, return +12C6) makes four more
draws and `PlayAmbient` (CODE 5, return +50BA) makes one. These account for
the captured entry-seed difference before the transition; no next-level
initialization draw is involved. `check_gameplay_exit_audit.py 13` verifies the complete exit
branches, caller mapping, native causal baseline and the five additional draws,
and rejects missing transitions and a corrupt final seed. The two
`manifest/gameplay-item16-exit-audit-13-*.sha256` manifests pin the observations.
This resolves the terminal level-thirteen RNG difference under D18 without
changing game decisions. The strict frame-count comparison remains failed.

MEASURED: current verification replay `.run-NeChbT` retains the two level-one
RNG differences at checkpoints 64 and 90. All 95 preceding checkpoints match
pixels, displayed palette, captured context and all six actor records; the strict
gate rejects level-two entry at frame 1000 / 1001 key reads versus original
1001 / 1002. The captured rejected entry matches pixels, palette, captured context,
actors and RNG. This is a failed strict route, not a newly accepted timing
exception. `manifest/gameplay-item16-level1-baseline.sha256` pins the evidence.
It uses the retained emulator for correctness only; no calibrated performance
claim follows. The corrected-clock attempt `.run-efa3Gx` was stopped during
slow verification startup without a route endpoint and is not a pass.

MEASURED: `.run-J9EmWq` attempts the full level-three route with current
inputs and trap tracing enabled, but stops at that same earlier level-two
entry. Every captured state, screen, palette and plane buffer reproduces
`.run-NeChbT`. Paired original/native exit branches begin waiting at step 918
and release at 1001/1000, respectively, with cue 9. The explicit
`check_gameplay_exit_audit.py 3 --first-level-control` verifies only this
level-one boundary and two missing-transition controls. The default level-three
audit rejects it with `TARGET LEVEL NOT OBSERVED`; it is not level-three exit
evidence. Matching the older diagnostic flags in current `.run-g0fds0`
(disabling trap tracing) produces the same earlier stop and identical retained
state. Both current probes remain incomplete for level three. The historical
level-three diagnostic `.run-glhUWS` uses its original pinned relocation
plan from revision `386bb98`; the other staged plans match current inputs.
`gameplay-item16-legacy-inputs.sha256` pins the private historical-input control.
Ordinary release inputs are not changed by this scoped replay.

MEASURED: that historical replay reaches the actual level-three exit, with
pending transition at 2695 in both versions and sound-gate release at
original 2767/native 2766. All level-three actor/RNG/counter, context, screen, palette and plane
captures reproduce `.run-Vz4heJ`, including its recorded differences from
the original. The sole difference
from that native baseline is the earlier level-one checkpoint 90 RNG, which
now matches the original exactly. The full original repeat, native build and
historical input identities, all retained states, exit branches and two
missing-transition controls pass `check_gameplay_exit_audit.py 3` with
`--native-manifest manifest/gameplay-item16-exit3-legacy-native.sha256`.
The default current-run check still rejects an unreached level-three probe.
This is evidence for the recorded historical difference, not a current-build
full-route pass or a rollback of production inputs.

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
| Level 1 | Meaningful gameplay route and original level-exit decision | Completed native/original pair; two pinned RNG exceptions under D11 |
| Level 2 | Meaningful gameplay route and original level-exit decision | Completed native/original pair; inherited level-one RNG exceptions only |
| Level 3 | Meaningful gameplay route and original level-exit decision | Accepted under D12: level-four entry captured; one-frame exit, torch/RNG/context and non-player actor differences explained and accepted under D18 |
| Level 4 | Meaningful gameplay route and original level-exit decision | Accepted under D12: all 93 checkpoints and level-five entry reached; recorded animation/RNG/context and actor-slot differences explained and accepted under D18 |
| Level 5 | Meaningful gameplay route and original level-exit decision | Clean assisted original repeat and native pair pass all 46 checkpoints and level six begins; the separate profiling route exercises carpet NIS 1 |
| Level 6 | Meaningful gameplay route and original level-exit decision | Accepted under D12: alternate door route and level-seven entry state match; one-frame exit difference explained and accepted under D18 |
| Level 7 | Meaningful gameplay route and original level-exit decision | Accepted under D12: route and level-eight entry state match; two-frame exit difference explained and accepted under D18 |
| Level 8 | Meaningful gameplay route and original level-exit decision | Accepted under D12: direct-entry route and level-nine entry state match; two-frame exit difference explained and accepted under D18 |
| Level 9 | Meaningful gameplay route and original level-exit decision | Original repeat and native pair pass all 15 checkpoints |
| Level 10 | Meaningful gameplay route and original level-exit decision | Accepted under D12: route and level-eleven entry state match; two-frame exit difference explained and accepted under D18 |
| Level 11 | Meaningful gameplay route and original level-exit decision | Accepted under D12: route and level-twelve entry state match; two-frame exit difference explained and accepted under D18 |
| Level 12 | Meaningful gameplay route and original level-exit decision | Accepted under D12: route and level-thirteen entry state match; two-frame exit difference explained and accepted under D18 |
| Level 13 | Meaningful gameplay route and original level-exit decision | Accepted under D12: level-fourteen entry captured; two-frame exit and recorded altar animation/context/RNG differences explained and accepted under D18 |
| Level 14 | Meaningful gameplay route and original level-exit decision | Assisted final encounter and original decision 15 paired through the ending and Hall insertion |
| Main cutscenes, individually | Original natural return and paired visible/state checkpoints | Selectors 1–11 individually paired; level-eight route accepted with the separately recorded D12 exceptions |
| Timed story scenes, individually | Original natural return for each selected scene | Selectors 12–20 individually paired through normal return; time-expired selector 20 included |
| Save/load | Save through session command, move, load, restored game state | Two original/native pairs; 22 rejection controls |
| Death | Original death routine returns after gameplay input | Existing first-room integration route |
| Restart | Original command followed by completed playable first frame | Original and native repeats pass; 15 rejection controls |
| Hall of Fame | Show/dismiss and persistent score insertion/readback | Original/native ending/name/write pair and fresh-session show/dismiss/readback pair pass |
| Ending | Final gameplay decision, ending return and subsequent session flow | Native/original ending route matches all 81 checkpoints and natural ending return, followed by successful Hall insertion |

DERIVED: CODE 2 `RestartGame` accepts levels 1–14; the session loop at
`CODE 2+45A4` selects the ending path for a result above 14. CODE 15's NIS
selector at `+0088` has separate main-cinematic and numbered story-scene paths;
these must be enumerated by their original dispatch and callers before claiming
complete cutscene coverage. These static facts establish scope, not execution.

## Cutscene dispatch scope

DERIVED and MEASURED: selector 5 is called through the exported PlayNIS entry
at A5+012A from CODE 9+071E. Its final-level room-two caller was bypassed by
the assisted ending route. `gameplay-princess-room-1` uses logged level, health,
room and position assistance, then executes the original trigger. Selector 5
returns normally and the following completed gameplay frame reaches room four.
Fresh `gameplay-princess-room-repeat-1` matches all 11 route checkpoints and
all nine cinematic capture files. The route has two key reads and nine logged
writes. `manifest/gameplay-princess-reference.sha256` and
`manifest/gameplay-princess-repeat.sha256` pin the original evidence.

MEASURED: native `.run-UvXvud` matches both cinematic copies and the normal
return, then the completed gameplay frame. All 11 route checkpoints pass,
including captured context, actors, RNG, pixels and displayed colours, with no
deferred differences. Restoration and the native launcher pass.
`manifest/gameplay-princess-native.sha256` pins the pair. The shared cinematic
checker requires both copy checkpoints for selector 5; its test rejects a
missing capture, altered pixel and missing normal return. The time-expired
story still passes the same checker.

DERIVED from original CODE 15 dispatch at +0088 and Transition at +07C8:
the numbered cinematic cases are 1–11. Cases 12–20 call TreeCheck with
indices 1–9. Out-of-range selectors take the default path, so a recorded
selector-100 return alone does not establish another completed cinematic.
The original procedure names identify these targets:

| Selector | Original target | Current acceptance |
|---|---|---|
| 1 | NISCarpet | Included in the passing clean level-five pair |
| 2 | NISTemple | Illustrated and normal-return checkpoints match in the completed A1200-030 survey |
| 3 | NISTower | Visible post-fade and normal-return checkpoints match in the completed A1200-030 survey |
| 4 | NISTitle | Existing paired title presentation plus natural timeout/black return pair below |
| 5 | TransPrincessRoom | Two cinematic copies, normal return and following gameplay frame paired within the completed room-two route |
| 6 | NISThroneRoom | Four visible/return checkpoints paired; containing level-eight route accepted under D12 |
| 7 | Opening1 | Unskipped normal return and illustrated/return checkpoint pair pass on A1200-030 |
| 8 | Opening2 | Natural return and three illustrated checkpoints match on A1200-030 |
| 9 | AboardShip | Four cinematic checkpoints and normal return paired within the completed level-one route |
| 10 | Discovery | Illustrated checkpoint and normal return match in the completed A1200-030 survey |
| 11 | HappyEnding | Included in the passing ending/Hall pair |
| 12 | TreeCheck index 1 | Illustrated and normal-return checkpoints match in the completed A1200-030 survey |
| 13 | TreeCheck index 2 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 14 | TreeCheck index 3 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 15 | TreeCheck index 4 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 16 | TreeCheck index 5 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 17 | TreeCheck index 6 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 18 | TreeCheck index 7 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 19 | TreeCheck index 8 | Three cinematic checkpoints and normal return match within the completed isolated assisted route |
| 20 | TreeCheck index 9 | Withered-tree cinematic and normal wrapper return paired through the time-expired checkpoint |

The table uses the actual CODE 15 argument. DERIVED from CODE 2+0308–0322:
the outer PlayNIS wrapper subtracts eight from story selectors before calling
CODE 15, so wrapper selector 20 means actual selector 12 (TreeCheck index 1).
Wrapper selector 100 invokes copy protection without calling CODE 15. Existing
`GAMEPLAY_ROUTE_NIS_*` records name outer selectors; cutscene-observer records
now sample the actual call sites at CODE 2+031E/+033E/+037E. Do not count a
wrapper-20 record as execution of TreeCheck index 9.

This table enumerates dispatch cases, not additional passes. Route entry/return
markers alone do not prove visible checkpoints match or identify whether a
scene took an early-return path.

### Unskipped opening observation

MEASURED: `gameplay-natural-opening-captures-3` runs Opening1 (selector 7)
without Escape, observes 5 CopyNIS and 21 NISTextDraw returns, and records its
normal result 0. The title (selector 4) then returns with result 32 after the
existing Space input, followed by level 1 room 4 and positive discovery
completion. Selector 8 is not called on this path; its coverage remains open.
`manifest/gameplay-natural-opening.sha256` pins the log and selected indexed,
palette and A5 checkpoints. Visual inspection confirms the text-8 checkpoint
shows the illustrated Princess/young-traveller passage; several other samples
are black during transitions. The paired native evidence below covers Opening1; this is not a full natural
title timeout.

The opt-in `POP2_CUTSCENE_OBSERVER=1` requires discovery mode and observes the
actual CODE 2 NIS call arguments before CODE 15 loads, then guarded CODE 15 copy,
text and return sites. An entry breakpoint in CODE 15 misses its first invocation;
the failed `gameplay-natural-opening-captures-1` is not accepted evidence.
The observer changes no original instructions or state.

MEASURED: native `.run-4ymwYZ` observes the same selector-7 result 0,
5 copy and 21 text returns, with zero Escape reads. Its text-8 illustration and
normal-return viewport indices and displayed colours match the original exactly.
`tools/check_gameplay_opening.py` verifies those captures, reference pins,
A1200-030 restoration, clean audits and positive route completion, and rejects
four pixel/colour corruptions. Native colour-table bytes pass through the existing
pinned video transfer table before comparison with Macintosh hardware RGB.
The first text/copy samples differ only within the visible Macintosh cursor box
(101,111)–(111,126); D8 removes that pointer on Amiga. These samples are retained
but are not the exact-checkpoint assertions.

`manifest/gameplay-natural-opening-native.sha256` pins the native evidence.
Reproduce with the verification integration build (CORE, DISPLAY, INPUT, UI,
AUDIO and INTEGRATION_PROBE), `AMIGA_SETUP=a1200-030 CORE=1`, and
`GDBSCRIPT=amiga/natural_opening.gdb` and `GAMEPLAY_OPENING_REFERENCE` set to
the absolute path of the pinned original run. The observer is read-only and records the
original cutscene sites after the segment-load callback. The run subsequently
reaches the first playable room and the existing death/restoration endpoint.
The recorded run used the generic core checker, which rejects its integration
completion under the default core-only expectation; the dedicated opening
checker passes. The launcher now selects it with `GAMEPLAY_OPENING_REFERENCE`. Title phase 64
versus the existing phase-16 title fixture is not accepted as a title match;
the selector-4 return pixels differ. Restore `make release` after diagnostics.

### Natural title and Opening2

MEASURED: original `gameplay-natural-attract-captures-1` and native
`.run-fYhJr4` run selectors 7, 4 and 8 without Space or Escape, then reach
level 1 room 4. Selector 4 returns 0 after its title timeout; selector 8
returns 0 after 43 CopyNIS and 23 NISTextDraw callbacks on both systems.
Opening2 copy-8, copy-32 and text-8 illustrated checkpoints, and its return,
match in viewport indices and displayed colours. The first-room 8,704-byte
context, actor records, level/room and RNG match exactly. Native restoration
and code/input audits pass with zero exception and zero Right/Space/Escape reads.

The title-return palettes match and both viewports render completely black.
Stored title-return indices differ in 78,821 pixels, so the checker explicitly
requires a black transition there rather than claiming an indexed match.
Visible title rendering remains covered by the existing state-paired title gate.
`tools/check_gameplay_opening.py --attract` checks seven checkpoints, lifecycle,
state and restoration, rejecting fourteen pixel/colour corruptions (including a
visible pixel inserted into the black transition). The evidence is pinned by
`manifest/gameplay-natural-attract.sha256` and
`manifest/gameplay-natural-attract-native.sha256`.

Original reproduction uses discovery and cutscene-observer mode with
`POP2_NATURAL_OPENING=1 POP2_NATURAL_TITLE=1`, followed by `finish` after the
first-room checkpoint. The recorded run used the older `POP2_DEMO_ROUTE=1`
input switch; its aggregate demo report rejected the missing second seed/demo
exit, which this first-room capture does not exercise. Its normal opening
report and dedicated paired checker pass. `POP2_NATURAL_TITLE` now suppresses
Space without selecting that unrelated demo-exit report.

Native reproduction adds `POP2_CUTSCENE_ATTRACT_PROBE` to the verification
integration flags, uses `amiga/natural_opening.gdb` on `a1200-030`, and supplies
`GAMEPLAY_ATTRACT_REFERENCE` as the absolute original-run path. The probe
requires verification and integration, withholds the diagnostic Space pulse,
and stops positively after the first room. Ordinary release has none of this
instrumentation. The recorded run predates launcher selection of the dedicated
checker and therefore ends with the generic core check rejecting its diagnostic
completion reason; the dedicated checker passes on its frozen evidence.

### Transition survey cutscene observations

MEASURED: `gameplay-survey-cutscenes-2` repeats every checkpoint of the pinned
assisted entry survey in captured context, actor/RNG state, counters, pixels and
palette, and positively completes all six observed CODE 15 calls. Actual
selectors are 7, 4, 10, 12, 2 and 3. Discovery (10) returns 0 after 7 copy and
15 text callbacks; TreeCheck index 1 (12) returns 0 after 1 copy and 4 text
callbacks; temple (2) returns 0 after 7 copies; tower (3) returns 0 after 5.
`manifest/gameplay-survey-cutscenes.sha256` pins the bounded captures and log.
Visual inspection confirms the tree text-4 and temple copy-4 illustrations.
The sampled tower copies are black, so they do not establish visible tower
coverage. Native comparisons remain pending.

The observer samples callbacks 1, 2, 4, 8, 32, 128 and 512 plus each normal
return, and requires every actual call to return before discovery completion.
The first survey observer run is rejected: its wrapper-based entry tracking
created an unmatched copy-protection entry and labelled the story by its outer
selector. The corrected actual-call probes exclude that branch and retain
byte guards without changing original state or control flow.

MEASURED: native `.run-ju4RWD` completes all 79 assisted survey checkpoints
(14 entries, 390 frames, 403 key reads) with no deferred mismatches, and observes
the same six cutscene returns. Discovery selector 10 text-8 and return captures
match the original viewport indices, displayed RGB, actor records, RNG,
level/room and story-index state. `tools/check_gameplay_survey_cutscenes.py`
reuses the full route checker and indexed/palette comparison, verifies the pinned
original and identical survey commands, and requires the normal Discovery return.
`manifest/gameplay-discovery-native.sha256` pins these native captures and
completion/restoration evidence. This establishes Discovery coverage; the other
new survey scenes still require their visible native checkpoints.

MEASURED: original `gameplay-survey-cutscenes-3` repeats the same 79 survey
checkpoints and adds early copy/text and completed fade-in observations. Native
`.run-qxYoR0` completes all 79 paired checkpoints without deferred mismatches.
TreeCheck index 1 text-4 and return, and temple copy-4 and return, match in
viewport indices, displayed RGB, actor records, RNG, level/room and story index.
`tools/check_gameplay_survey_cutscenes.py --expanded` verifies these pairs along
with Discovery and full route completion/restoration. The expanded original and
native manifests are `manifest/gameplay-survey-cutscenes-expanded.sha256` and
`manifest/gameplay-survey-cutscenes-expanded-native.sha256`.

The observer also samples FadeInBothCTBLs returning at CODE 15+2A28. Tower uses
a direct palette fade, so that convenience-routine probe does not supply its
visible checkpoint. Original `gameplay-survey-cutscenes-4` adds the direct fade's
return at CODE 15+72DC and again repeats the entire survey. Its tower capture
visibly shows the tower and sky; its hash-pinned evidence is in
`manifest/gameplay-survey-cutscenes-tower.sha256`. The native pair is recorded below.

`tools/gameplay_cutscene_observer.py OUTPUT` combines the existing route observer
with the cutscene commands from `amiga/natural_opening.gdb`. Use the generated
file as `GDBSCRIPT` with the ordinary verification assisted-survey build and
`GAMEPLAY_LEVEL=0`. Each launch freezes its observer and build; captures are
read-only, and the regular route checker still requires all 79 paired checkpoints.

### Throne-room cutscene reference

MEASURED: `gameplay-level8-cutscenes-1` repeats every checkpoint of
`gameplay-assisted-level8-clean-2` with the same logged assistance and direct
keys, then enters level 9 room 10 at step 490. Its actual selector-6 call returns
0 after one CopyNIS and nine NISTextDraw callbacks. Visual inspection confirms
the ruined throne room after fade-in, and the restored room with the apparition
at text-4 and text-8. The observer accounts for all three actual NIS calls and
positively finishes discovery. `manifest/gameplay-throne-reference.sha256`
pins the cutscene captures and route log.

MEASURED: native `.run-M9Mmmf` matches the original fade-in, text-4, text-8
and return viewport indices, displayed RGB, actors, RNG, level/room and story
index. Selector 6 returns normally with the same callback counts. The checker
`tools/check_gameplay_throne.py` verifies those four captures, build identity,
A1200-030 restoration and code/input audits. Native evidence is pinned in
`manifest/gameplay-throne-native.sha256`.

This is a cutscene pass only: the containing route stops at `GAMEPLAY ROUTE /
FIRST FRAME`, entering level 9 at step 489 rather than the original's 490.
The level-eight timing/context differences remain open; the cutscene checker
reports that separate route outcome and does not accept the full route.

MEASURED: native `.run-JsjtjO` matches the original tower post-fade and return
viewport indices, displayed RGB, actors, RNG, level/room and story-index state.
It returns normally from selector 3 after five CopyNIS callbacks, and completes
all 79 survey checkpoints with no deferred mismatches and clean restoration.
`tools/check_gameplay_survey_cutscenes.py --tower` checks Discovery, tree index 1,
temple and tower together against `gameplay-survey-cutscenes-4` and the pinned
survey route. `manifest/gameplay-tower-native.sha256` pins the tower captures and
native completion evidence. The tower's visible checkpoint is the guarded return
from its direct fade at CODE 15+72DC, not an earlier black CopyNIS sample.

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
route. This diagnostic gate covers one session. The separate ordinary-release WHDLoad
fresh-boot save/load gate now passes; see `release.md`.

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

At a paused gameplay boundary, `N save SLOT` saves an emulator checkpoint.
`N load SLOT` is explicitly **unsupported**: the controller reports
`GAMEPLAY_STATE_LOAD_UNSUPPORTED`, records the saved/current key names in
`route-state-rejections.txt`, and publishes a `state-load-rejected` checkpoint.
It consumes the command sequence number but does not load the state, change
input, or advance the game. The current route remains usable. This applies to
all loads, including neutral-to-neutral and identical held-key cases; there is
no verified safe subset. Use a fresh route or ordinary in-game save/load instead.

Slots use letters, digits and hyphens, up to 48 characters. Existing slots are
never overwritten. States stay in the ignored run folder under `states/maciici/`.
A load request still requires a slot created in the same controller session,
outside an active cutscene. Sequence numbers continue increasing so captures and
command history are retained. Saved emulator states are diagnostic artifacts,
not supported retry points. Ordinary game saves remain level checkpoints.
Successful routes require fresh fixed-seed original replay without emulator
restores and paired native execution for acceptance.

DERIVED: the old restore check covered CPU registers and full A5/context bytes,
but did not verify host input overrides or pending ADB transitions. Reapplying
key names cannot establish that the next original input poll sees the same
state. INFERRED: this gap can explain the observed dependence on preceding held
keys; the specific emulator mechanism is not established. Neutral keys do not
prove that an earlier transition has been consumed. Vette! and Alone in the
Dark both use ioport `set_value` for held input and provide no state-restore
continuity contract. Their helpers and the shared input helper are unchanged.

MEASURED: `restore-guard-1` exercises five rejected load requests: neutral to
neutral, held Left to held Left, neutral to held Left, held Left to neutral,
and neutral after a held-key transition. Every rejection preserves full A5,
context, indexed screen and palette bytes from the immediately preceding
checkpoint. The original then completes three requested gameplay frames and
explicitly finishes discovery. Two saves also preserve their checkpoints.
`tools/check_discovery_restore.py --controls ref/mame/runs/restore-guard-1`
checks this evidence and rejects five mutations covering key reports, command
coverage, completion, context and frame count. The names and hashes are pinned
in `manifest/discovery-restore.sha256`.

To reproduce, start an isolated reference with `POP2_DISPLAY_ROUTE=1`,
`POP2_NATURAL_OPENING=1`, `POP2_GAMEPLAY=route` and `POP2_ROUTE_DISCOVERY=1`
using `tools/run_reference.py --name NAME --deadline 900`. In another shell run
`python3 tools/check_discovery_restore.py ref/mame/runs/NAME --drive`.
Both commands must finish successfully. This tests the unsupported-condition
fallback, not repaired keyboard continuity; fresh-original route gates remain
mandatory.

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

### Level-one ambient scheduling and retained strict comparisons

MEASURED: the diagnostic A1200-030 run `.run-rXZcAu` reaches level 2, room 2
at step 1001, with 1002 matching consumed key maps and successful code, input
and hardware-restoration audits. All 96 checkpoints match the reference's
full level context, six actor records, level/room/step counters, displayed
pixels, eight bitplanes, palette and Copper configuration. The pair is not
accepted: the QuickDraw random seed differs at checkpoints 64 and 90. In both
cases the reference is one random draw ahead; later checkpoints match again.
The read-only random-call observations below identify the ambient scheduling
shift. D11 scopes the retained hash exceptions; D18 accepts the verified timing
consequences. The strict comparison still reports them.

MEASURED: a second original route reproduces the pinned checkpoints with a
read-only random-call observer. `PlayAmbient` (CODE 5+4F92) consumes the next
random value at steps 526 and 823; the corresponding native trace consumes
the same values at steps 544 and 854. The ambient record at A5−3738 also
differs at the rejected endpoints, then matches again. This is an audio-state
difference, not merely an unused seed. The production effect-lifetime and queued-clock comparisons above distinguish
logical effect duration from asynchronous scheduling. D18 accepts the measured
consequences without requiring identical wall-clock phase.

MEASURED: `gameplay-level1-audio-3` completes the original level-1 route with
its audio observer and retains all 96 pinned route checkpoints. A separate
original-driver fixture, repeated in `audio-callback-order-1` and
`audio-callback-order-2`, replaces an active effect
with an authored descriptor. The old effect's completion callback changes
the replacement ID from 5 to 7; selector 20 reports 7 playing and 5 stopped,
with exactly one callback. Native `.run-xisZJy` matches both original runs:
seven completed calls (including native initialization), one callback, two
Paula starts, all seven register-preservation checks and restored hardware.
The checker rejects 18 corrupted completion, callback, ID, query, ABI, build
identity and hardware records.

The replacement service stops the old effect and runs its callback through
the existing user-mode trampoline before retrying the original call. It reads
the replacement descriptor only after that callback returns. The fixture's
sample allocation precedes resource lookup so it cannot retain a stale pointer
across heap compaction. The full level-1 replay with this fix (`.run-rUIRoc`) still rejects
checkpoints 64 and 90 for RNG. All 96 captured pixels, bitplanes, palettes,
0x2200-byte contexts, actors and level/room/step counters match. The ambient
random calls occur at native steps 544 and 855, versus original 526 and 823.
The callback correction therefore does not resolve gameplay timing equivalence.

Recheck the focused pair and its rejection controls with
`python3 tools/test_audio_callback_order.py amiga/.run-xisZJy ref/mame/runs/audio-callback-order-1`.
The original fixture is explicitly selected with `POP2_AUDIO=fixture
POP2_AUDIO_CALLBACK_ORDER=1`; the native fixture requires verification plus
`POP2_AUDIO_PROBE` and `POP2_AUDIO_ORDER_PROBE`. Ordinary release builds contain
neither fixture.

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
restores were used for retries; new restore requests are now rejected. `tools/gameplay_routes/level2.tsv` records the
successful 19-action path. The level-three first frame is saved as
`level3-start` in that session. This is discovery evidence, separate
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


### Level-two comparison with inherited ambient exceptions

MEASURED: `.run-ITSYlT` executes the complete pinned input stream through levels
one and two on A1200-030, reaching level 3, room 1, at step 1360. It consumes all
1362 original key maps, including both reads of step 1001, captures all 115
checkpoints, and restores code, input, heap and hardware successfully. All 20
level-two entry/action checkpoints match the original context, actors, RNG,
level/room/step counters, pixels, AGA planes, palette and Copper configuration.
The combined route remains rejected at the same two level-one RNG checkpoints
(64 and 90). The checker evaluates later checkpoints while retaining those
failures. The inherited ambient scheduling differences retain their strict
failures and are explained under D18 above.


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

The first continuing discovery session ended at its one-hour host deadline;
its captures and compressed disk are preserved. A new longer bounded session,
`gameplay-level3-discovery-1`, replays the pinned earlier levels and resumes
cave-route discovery. `run_reference.py --deadline SECONDS` permits an explicit
host deadline up to 12 hours; expiration remains failure, never completion.

### Level-three discovery completion

MEASURED: `gameplay-level3-discovery-1` reached the original level-four first
frame at step 2767, room 1, after level-three exit gameplay and a natural return
from story selector 10. Checkpoint `route-236` records `reason=next-level`;
`route-237` preserves the level-four starting state for further discovery.
This run used emulator checkpoint restores. Its successful branch is recorded
as 66 ordinary-input intervals in `tools/gameplay_routes/level3.tsv`; abandoned
attempts are excluded. Fresh replay is verified below; native pairing remains pending.

### Level-four original repeatability

MEASURED: `gameplay-level4-fresh-1` and `gameplay-level4-fresh-2` replay all
265 authored input intervals from fresh Macintosh startup without emulator
restores. All 266 gameplay checkpoints match captured context, six actor records,
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

MEASURED: `gameplay-level5-discovery-3` freshly replays all 265 authored
level-one-through-four intervals against the pinned level-four reference.
The original Restart command subsequently returns to level five, room one
(`route-303` and `route-352`), allowing local retries without replaying earlier
levels. These are discovery observations, not the full restart acceptance gate.

MEASURED: the entrance route knocks down the first skeleton and crosses the
lava. A standing jump from x 565 with one frame of right/up followed by 30
neutral frames lands in room two at x 220 with all three health points
(`route-403`). Two careful steps and the left-hand climb reach the upper
platform without activating the lower switch (`route-409`). Holding jump
longer carries the prince farther into the room and changes the approach.

MEASURED: after backing off and activating the upper switch again, both doors
are open (`route-430`, two health points). Careful steps to x 533 followed by
one frame right/up and 30 neutral frames cross the pit (`route-437`).
`route-438` enters room nine; `route-440` saves `trap-crossed` at x 317,
y 226, with two health points. `landing-no-lower-switch`,
`upper-no-lower-switch` and `both-doors-open` preserve intermediate discovery
checkpoints. The corridor beyond reaches the carpet in room three;
`route-446` saves `carpet-room-arrival` with two health points.
MEASURED: discovery continues through rooms four, five, eight and thirteen,
including both potion pickups and the ledge catches. `route-532` saves
`lower-gate-approach` in room fourteen with two health points. Its gate opens
after a 60-frame neutral interval (`route-533`). The lava-room crossing uses
an upper-platform jump followed by a second jump from the landing animation;
`route-584` saves `lava-room-exit-alive` in room sixteen with two health points.
The subsequent climbs reach room seventeen (`route-602`, `upper-shaft-entry`).
MEASURED: the room-seventeen right-edge hang and climb leave the upper-left
gate open (`route-778`–`route-780`). The return jump catches the upper-left
ledge (`route-784`); climbing and jumping left reaches room twenty alive
(`route-788`). Releasing movement on arrival gives a stable two-health-point
checkpoint at x 570, y 106 (`route-799`, saved as `room20-upper-entry`).
A jump and ledge climb cross room twenty (`route-803`–`route-806`). A shorter
leftward approach avoids running into room twenty-five's blade: `route-818`
stops at x 689 with two health points. The upward route reaches room six
(`route-823`), whose two ledges lead right into room eleven (`route-832`,
saved as `upper-east-corridor`). Its next jump catches the right middle ledge
(`route-844`) and climbs onto it (`route-846`), still with two health points.
The direct running-jump branch from x 286 (`route-962`–`route-970`) clears
room eleven with both health points; the longer central-platform branch does
not establish that passage. From its upper exit, 16 right, 10 right/up and
18 right/shift catch room eighteen's upper-right ledge (`route-1008`). The
climb completes at `route-1010`; further upward climbs reach room twelve
(`route-1026`). Clearing its loose slab and climbing again reaches the upper
ledge (`route-1041`, one health point). The left jump lands by the bridge
entrance (`route-1047`); `route-1050` reaches the living skeleton in room ten.

MEASURED: death in that fight followed by gameplay input restarts at the
original room-twelve checkpoint with all three health points (`route-1063`
and `route-1075`). This supports local fight retries; it is not a fresh paired
restart acceptance result. From the natural restart at `route-1105`, 30
left, 12 neutral, then three eight-frame left presses separated by four
neutral frames pass the skeleton (`route-1112`). The prince turns right
on its left side with two health points. `route-1116` saves this as
`bridge-sides-swapped`.

DERIVED: `BridgeCheckBreak` at CODE 8+002E requires both fighters on the
bridge, their columns no more than three apart, a counter of at least 60,
the prince in columns 4–6 and the opponent in columns 5–6. The earlier
stable duel at x 387 leaves the prince in column 2 and cannot trigger the
collapse. This explains the navigation requirement without changing game logic.

MEASURED: from `bridge-sides-swapped`, eight down, twelve right, eight
control, twelve neutral, eight up and eight control reach x 464 with two
health points (`route-1199`, saved as `bridge-central-safe`). Four left,
then two repetitions of eight up/eight control and another eight up reach
x 448 (`route-1246`, saved as `bridge-left-before-collapse`). Four left,
one neutral, four left and twelve shift catch the surviving left edge
(`route-1251`, frame 90, two health points). Thirty up/shift and twelve
neutral complete the climb (`route-1253`, standing at x 355, y 230).
`bridge-collapse-survived` preserves this discovery checkpoint. The bridge
state is −1 after the collapse; the original skeleton has fallen out of the room.

MEASURED: twenty left and twelve neutral enter room seven alive
(`route-1256`). Its exit gate remains closed on this branch; crouching,
careful movement and a running crouch do not pass it. The potion can restore
a health point (`route-1333`), but the subsequent floor/dart passage does
not open the gate. The approach-switch hypothesis is unproven. Re-entering
room twelve from the natural checkpoint permits jumping to the right-hand
platform, climbing its upper ledge and returning with all three health
points; this preserves an alternative approach for further discovery.
The carpet gate and level exit remain incomplete.

The successful branch includes emulator restores and requires
fresh original repeats before acceptance. No level-five completion or native
comparison is claimed.

External route guidance: the owner supplied the Macintosh longplay
[Prince of Persia 2 Macintosh Longplay Playthrough](https://www.youtube.com/watch?v=fXPzKBH4j78&t=750s).
Its 12:30–12:51 passage shows the direct left-hand climb, upper-switch retry
and door crossing. The video provides navigation guidance, not exact input
records or fixed-seed acceptance evidence.

Restored discovery attempts have sometimes diverged after identical short
commands when preceding held input differed. The discovery controller now
explicitly rejects emulator-state loads, as described above. Only fresh repeat runs can establish route acceptance.

### Level-three original repeatability

MEASURED: `gameplay-level3-fresh-1` and `gameplay-level3-fresh-2` execute the
authored route from fresh Macintosh startup without emulator restores. All 181
gameplay checkpoints match in captured context, six actor records, RNG, counters,
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
coverage is accepted under D12; D18 explains the recorded timing/state
differences below. The earlier strict RNG comparisons remain failures.

### Level-three comparison and causal RNG fixtures

MEASURED: `.run-oMs5lv` on A1200-030 captures checkpoints 0–179 and reaches
the level-four first-frame hook, but the route gate rejects that entry: native
step 2766 and 2769 consumed keys differ from the original's step 2767 and
2770 keys. The run reports `GAMEPLAY ROUTE / FIRST FRAME`; it is not a pass.
Original-code, input, heap and hardware restoration pass, and the ordinary
release build is restored afterward.

MEASURED: `.run-Vz4heJ` reproduces all 180 retained native captures in screen,
captured context and curated actor/RNG/counter spans, then captures the rejected
level-four entry in room 1. Counts remain 2766 frames/2769 key reads versus
original 2767/2770. The endpoint palette matches; pixels, actors, RNG and full
context differ. Context differences are at 0x07D9 and 0x07F1; 326 viewport pixels
differ within x168–480/y276–312. Code/input audits and restoration pass.
`manifest/gameplay-level3-rejected-entry.sha256` pins this diagnostic evidence;
its report explicitly retains `route_accepted: false`.
The endpoint player record at A5 minus 0x50D2 matches in full. Actor differences
are confined to zero-based slot 0. The seed fixture below establishes their
RNG-dependent position and animation consequences. Visual comparison places the pixel differences in the two
wall torches. These observations do not extend the owner-approved exception.

The first additional level-three mismatch occurs at checkpoint 121. Checkpoints
114–120 match captured context, actors and RNG; the frame verifier passes their
pixels, planes and palette. At checkpoint 121 all six actor records still match,
but RNG, three context bytes and torch pixels differ. Later checkpoints retain
pixel/RNG differences. The prior level-one RNG failures at 64 and 90 also
remain. These observations locate the first divergence; the paired RNG traces
and original counterfactuals below establish its cause and gameplay effects.

MEASURED: the explicit random-call observer in `gameplay-level3-random-1`
completes the full three-level route with normal exit and matches all 181
checkpoints and 2770 keyboard reads despite observing the calls. Within
steps 1490–1545, the original makes 161 calls from `CaveGetTorchFrame`
(CODE 8 return offset 3B5C), three from CODE 5 return offset 205E, and one
from `PlayAmbient` (CODE 5 return offset 50BA) at step 1515.

MEASURED: `tools/check_gameplay_random_window.py` pairs that bounded window
with native `.run-845iXe`. Both produce the same first 164 RNG seeds and
results in call order. The original makes one additional draw at step 1515,
from `PlayAmbient`. Excluding this call, the per-frame call counts and order
match through step 1545: 161 calls in the torch class and three in the other
class. The original's extra draw consumes the value otherwise received by
the next native call. Native caller-class correspondence is INFERRED from
these unique counts and per-frame order; that older observer did not capture
its segment map. This establishes a shifted shared RNG stream, not a defect
in the RNG sequence itself or equivalence of subsequent actor decisions.
Two missing/altered-evidence controls reject, and
`manifest/gameplay-item16-random-window.sha256` pins the inputs. The native
run retains its failed level-four entry and successful code/input/restoration
audits. The causal fixtures below establish the resulting torch and actor
consequences; this bounded call trace alone does not supply that proof.

MEASURED causal control: `item16-rng-rollback-1` repeats the unchanged original
through checkpoint 120. At the guarded `PlayAmbient` draw at step 1515, the
opt-in fixture preserves its returned choice and restores only the preceding
RNG seed. At checkpoint 121 (step 1542), the original then matches native
`.run-845iXe` in viewport pixels, all eight planes, palette, captured context,
all six actor records, RNG and route counters. The unchanged original at the
same checkpoint differs in RNG and context. This isolates the extra ambient
draw as the cause of the first torch divergence; it does not accept the rest
of the route. The original emits the single-write and completed-fixture markers
and exits normally. `tools/gameplay_rng_fixture.py` derives its ignored inputs
from pinned observations; `tools/replay_gameplay_rng_fixture.py` verifies the
unchanged prefix and changed endpoint. Evidence is pinned in
`manifest/gameplay-item16-rng-rollback.sha256`. Ordinary play is unchanged.

The first level-three actor mismatch is earlier than the rejected final entry:
checkpoint 134, step 1877. Slot 0 differs in x position and words +10, +36,
+38 and +40. At checkpoint 133 (step 1865), all six actor records still match
despite the differing RNG seeds. Both checkpoints place the player and this
actor in room 17. This is a character-state difference during gameplay, not
just residual data in the next-level capture. The paired seed fixture below
establishes its RNG causality.

### Assisted entry survey

Owner decision D10 permits logged assistance to bypass gameplay obstacles.
`POP2_ROUTE_ASSIST=1` additionally requires discovery mode. At a stopped original
frame, `N assist health VALUE` changes living-player health and maximum health
(1–12), `N assist x VALUE` changes horizontal position (230–720), and
`N assist level VALUE` requests the original restart/load path for levels 1–14.
Every write records its old/new word and has separate before/after captures.
The level request uses the session fields consumed by CODE 2+6124 and +7148;
it does not alter the timer. Assistance is excluded from ordinary release.

MEASURED: `gameplay-assisted-survey-3` loads levels 6, 2–5 and 7–14 from a
fresh level-one start and completes 30 original gameplay frames in each.
Its 65 logged word writes are checked against complete before/after A5 captures;
context and pixels remain unchanged across each intervention. The session
returns normally after an explicit discovery finish. These are assisted entry
and short gameplay observations, not natural exits or full-game acceptance.
`manifest/gameplay-assisted-survey.sha256` pins the private evidence by hash.
The independent `gameplay-assisted-survey-repeat-1` repeats every checkpoint
in captured context, actor records, RNG, counters, pixels and palette. A second repeat
with the handle observer also matches. Native run `.run-lgnTYZ` reaches 27
checkpoints, then stops while entering level 5 at CODE 15+29C2, HUnlock, with
`HEAP / UNKNOWN HANDLE`. Restoration passes. The measured disposed-palette HUnlock is now handled
only for its verified caller and free master slot: the original returns
`memWZErr` and preserves that slot. `check_handle_state_contract.py` checks
both the live scene and the distinct isolated-handle fixture. Native
`.run-6MxUx9` passes that stop and reaches 51 checkpoints before a same-buffer
CopyBits stop. `.run-jy7hKD` passes the copies and stops next at
`DISPLAY / NEWGWORLD BOUNDS` (CODE 17+4E1A). Both restore normally;
no native survey completion or item-12 acceptance is claimed.

`tools/replay_gameplay_assisted.py RUN` repeats the pinned intervention/input
sequence. `tools/gameplay_route_inputs.py --level 0` generates its private native
replay data. The explicit verification route applies writes only at matched
frame/key/level/room boundaries, checks each original prior value, and captures
both sides. Invalid intervention ownership, unlogged state changes and altered
pins reject. Existing unassisted route checks retain their separate expectations.

The original `gameplay-assisted-copy-4` repeats the entire assisted entry
survey and captures eight same-buffer copies at CODE 17+508A. Their source and
destination rectangles are disjoint horizontal bands within one 512×384 buffer;
the previous runtime rejected all same-buffer operations as overlap.
`tools/check_copybits_overlap.py` checks all output bytes against the shared
helper, with private evidence pinned in `manifest/gameplay-self-copy.sha256`.
The shared implementation follows Vette's direction-aware traversal and also
passes 361 synthetic clipped overlapping copies in both directions. AmigaXDev
commit `bacd172` retains the current shared helper's clipping and row-loop
optimizations. Masked, remapped, scaled and unequal-stride aliasing still reject.

Ending discovery reaches and returns from the princess/Jaffar scene (selector
5). Six-frame alternating turns with health assistance produce the shadow;
the final-level flame predicate requires health greater than two. This
is discovery evidence, not ending acceptance. The forced room-3-to-room-4 jump
in `gameplay-assisted-ending-4` caused the original to malfunction and is
excluded. Remaining final-room travel must use original transitions; a room
number change alone does not reconstruct the level's scene state.

`gameplay-assisted-world-1` repeats the assisted survey and captures 48
successful offscreen allocations. `tools/check_gameplay_worlds.py` checks every
non-pointer PixMap field in all seven layouts, with the input bytes pinned in
`manifest/gameplay-worlds.sha256`. Later cutscenes add 510×365, 365×365,
239×353 and 510×225 to the startup layouts. Their main-device rowBytes are
respectively 528, 384, 256 and 528. The shared GWorld helper now accepts the
measured sixteen-byte rounding/slop policy separately from its unchanged
four-byte keepLocal default. The runtime still rejects unmeasured bounds.

Native `.run-P6joXU` passes the measured allocation requests and next stops at
CODE 20+085A, `HandToHand` (A9E1). The original authored
`gameplay-hand-copy-2` fixture copies a locked/purgeable sixteen-byte handle,
checks every copied byte, distinct master/data addresses, unchanged source,
new handle size/state, and the result after deliberately setting CCR and
MemError before the call. It returns D0/MemError zero, CCR Z only and an unlocked
new handle. `check_handle_copy.py` verifies the pinned fixture; the shared
`HandleCopy.h` adapter also tests allocation failure without purging the source.
Unknown/purged source handles and unmeasured allocation errors remain loud stops.

Assisted discovery may opt into `POP2_ROUTE_DIRECT_KEYS=1`. At the observed
keyboard-read return it supplies and logs the explicitly requested KeyMap,
avoiding asynchronous ADB hold timing during assisted navigation. This is a
validation-only input intervention; ordinary play and existing ADB references
are unchanged. `assist shadow 1` is restricted to a live actor in level 14:
it sets the measured shadow kind/display attribute and two health points.
It deliberately does not create the abandoned-body actor, so these runs do not
claim body re-entry coverage. All interventions still require paired replay.

### Assisted final encounter and ending

MEASURED: `gameplay-assisted-ending-10` starts fresh, uses logged level,
position and health assistance, clears the crystal doubles and follows original
room transitions to the final encounter. The original flame attack defeats
Jaffar. CODE 2+45B0 observes the session result 15; NIS selector 11 enters and
returns naturally to CODE 2+45B8. The route uses no emulator restores and finishes
at 385 gameplay steps, with 387 consumed KeyMaps, 81 captured checkpoints and
41 assisted word writes. `manifest/gameplay-assisted-ending.sha256` pins the
private evidence. Its final image is black after the cinematic fade; this
endpoint alone does not establish every intermediate cinematic frame.

The observed flame predicate at CODE 4+2590 requires health **greater than**
two in the final level (or special state 13). The assistance creates an ordinary
shadow with two health points; a later logged health increase enables the flame.
The earlier restored discovery run is not fresh-repeat acceptance. Hall of Fame
and subsequent session flow remain separate, unfinished checks.

The original `gameplay-assisted-cursor-3` repeats the entry survey and observes
fourteen successful GetCursor returns and the following HUnlock/HPurge cleanup.
System 7 returns its ROM-backed wait cursor. D8's transparent Amiga replacement
uses a heap handle instead: later cutscenes may purge it. GetCursor now reloads
that owned replacement through the existing heap reallocation service when its
master becomes empty; unknown handles and wrong-size live handles still stop.
Native `.run-oWaONb` passed HandToHand and exposed this cursor stop; its restored
hardware/heap and code/input audits pass. The reload regression is in progress.

The independent `gameplay-assisted-ending-repeat-1` matches all recorded ending
route checkpoints in pixels, palette, full level context, actors, RNG and counters.
Native `.run-ZtIfH0` matches the first 62 checked checkpoints before its next stop;
this is partial route coverage, not an accepted ending pair.

Native `.run-8I8IDt` passes the cursor reload and reaches level ten, then encounters
EqualRect (A8A6) while entering level eleven. The original
`gameplay-equal-rect-1` fixture passes 36 true/false cases, including empty,
inverted and signed-boundary coordinates; it preserves both inputs and the
Boolean result slot's padding byte. `check_equal_rect.py` checks the pinned
results and the shared helper with sanitizers. AmigaXDev commit `cfeff00` supplies
that helper; native regression remains pending.

The final-tower climb in native `.run-ZtIfH0` stops at BitMapToRegion (A8D7,
CODE 17+5E3A). `gameplay-assisted-bitmap-2` repeats the full ending route and
captures 300 successful original conversions. `check_bitmap_region.py` matches
every encoded region byte with the shared helper, plus extent rejection controls.
The shared row storage now accommodates the intermediate XOR states needed by
these masks; the latest shared validation cache is retained. AmigaXDev commit
`7b31ab6` provides the helper and sanitizer tests. The earlier observer run stopped
at its diagnostic capture limit and is excluded from route completion evidence.

Native `.run-vJ1OWY` passes bitmap conversion and stops at OffsetRgn (A8E0,
CODE 17+5E68). `gameplay-assisted-offset-1` completes the original ending and
repeats every pinned route checkpoint; all 300 captured region translations
byte-match the existing shared `RegionOps::map` helper under sanitizers.
`manifest/gameplay-offset-region.sha256` pins the call evidence. The native
adapter uses that mapper; the adjacent PaintRgn path reuses Alone in the Dark's
shared region-intersection and masked solid-fill approach. Native acceptance
of those adapters is pending.

`gameplay-assisted-hall-4` positively completes the original ending, name entry,
confirmation, score write and Hall return, then explicitly flushes the volume.
The exported file contains name `codex` and score 75. The original retains
uninitialized name padding, so cross-platform comparisons use count/name/score
rather than treating padding as meaningful state. The original-written file,
name-table captures and endpoint log are pinned in
`manifest/gameplay-hall-reference.sha256`; `check_gameplay_hall.py` verifies them.
The port-owned name dialog and keyboard-driven native Hall fixture remain under
validation. This evidence does not yet establish native Hall acceptance or the
separate show/dismiss/readback gate.

The fresh original `gameplay-assisted-hall-repeat-1` repeats the complete pinned
ending route and all four Hall endpoints; its accepted and flushed name/score
match. The native `.run-kn5zMO` passes both region adapters and matches its first
68 captured checkpoints, then stops at ResrvMem (CODE 2+721C). Its previous
adapter only compared the current largest free block. It now invokes the shared
heap's compaction/purge operation, preserving locked and non-purgeable handles;
the native ending/Hall replay is still pending.

Native `.run-Gxam8n` reserves 128000 bytes successfully: the largest free block
grows from 8176 to 184872 bytes. Its first 80 captured checkpoints match original
pixels and full level state, including the final gameplay frames. The ending
then stops at PaintRect (CODE 15+3CAC) on the newly created port's pen pattern;
the complete ending and Hall native endpoints remain unaccepted.

The updated entry survey `.run-b29glb` positively completes all 14 loaded levels,
390 game frames and 403 consumed key maps. All 79 checkpoints match full level
context, actor/counter state, RNG, indexed pixels, eight bitplanes, palette and
Copper output; code, input, heap and hardware restoration pass. This is entry
and short-play coverage, not a claim that the original exit puzzle of every
level has been exercised.

D11 permits the previously recorded level-one RNG exceptions; their ambient
scheduling consequences are documented under D18 above. `check_gameplay_route.py --deferred-rng` accepts only the exact pinned
native/original seed hashes at checkpoints 64 and 90, only for the completed
level-one/two routes. It still requires all other state/pixel checks and positive
completion. Native `.run-rXZcAu` and `.run-ITSYlT` pass that explicitly qualified
gate; their reports retain both exceptions. The level-three early-exit counter
difference and torch captures have not been silently accepted by this option.
The D11 corruption controls reject a changed RNG value, a changed in-game pixel
at an exempt checkpoint, and missing completion; the unchanged control passes.

`gameplay-assisted-paint-2` confirms that the ending PaintRect uses a type-zero
solid-black pen (eight one-bit data bytes set), patCopy and foreground index 255.
The original paint and ending both return successfully. The port now initializes
its owned monochrome pen/fill defaults before PenNormal and uses the pat1Data
field at offset 20. `check_ending_pen.py` checks the pinned pen-state evidence.
The observer's attempted video dumps used virtual access to physical video RAM;
those dumps are excluded from the manifest and are not pixel evidence.

Native `.run-hclZeL` passes the default pen and stops on a later ending GWorld
size outside the old whitelist. The adapter now delegates valid bounds to the
existing shared layout validator. `gameplay-assisted-ending-worlds-1` completes
the original ending and repeats every pinned checkpoint; its 36 allocations
match the helper's non-pointer PixMap fields. The observed layouts include the
ending's additional dimensions, pinned in `manifest/gameplay-ending-worlds.sha256`.
The host test exercises every supported width for the 16-byte layout and rejects
the first stride overflow.

Native `.run-NplQMc` passes the ending GWorld allocation and retains matching
pixels/captured context at all 80 pre-ending checkpoints. It then stops at HUnlock
(CODE 15+29AA) during cleanup. The original `gameplay-assisted-ending-handles-1` returns `memWZErr`
without changing the alias and completes the ending with all pinned checkpoints
matching. The existing free-master-slot handling now includes this measured
caller; its log is pinned in `manifest/gameplay-ending-handles.sha256`. The endpoint
and Hall gate remain incomplete. The complete host scaffold suite and ten
Python gameplay-input/evidence controls pass with the current changes.


The original assisted level-four route `gameplay-assisted-level4-1` reaches the
natural level-five entry after 1889 frames. Health assistance is recorded; no
exit result was written. A fresh repeat and native pair remain pending.
Native `.run-nb7R9n` passes the first ending cleanup alias and stops at the third
(CODE 15+29DA). The same completed original ending-handle trace records the
unchanged slot and `memWZErr` for that caller too; the narrow adapter now covers
all three measured cleanup sites. This subsequent native endpoint is pending.

The owner requested portable saves for profiling. Original `profiling-saves-1`
writes and flushes all 14 level-entry saves through SaveGame. They are exported
unchanged under ignored `tmp/profiling-saves/`, with names/hashes pinned in
`manifest/profiling-saves.sha256`. These are profiling inputs, not completed
level-route evidence. See `docs/profiling-saves.md` for release staging and use.

Fresh original `gameplay-assisted-level4-repeat-1` matches all 93 checkpoints
of the pinned assisted level-four route, including its natural level-five entry.
The native replay is prepared but has not yet run.

Native `.run-Of8g4k` positively reaches the original ending return: 385 frames,
387 key reads and all 81 checkpoints match pixels and captured context. The next
Hall drawing call stops at FrameRect (CODE 2+0D20), so the combined Hall gate
still fails. D8 assigns that Macintosh dialog rendering to the port-owned UI;
the pending replacement draws the score table with the existing overlay and
skips the old accepted-name glyph redraw while retaining original ranking and
WriteHallOfFameFile. No generic FrameRect success is fabricated.

Native `.run-2OVibg` completes the 81-checkpoint ending pair and reaches the
Hall return after six name-edit events and one confirmation. Its persisted
`Data/Prince.hof` is empty: the semantic persistence checker fails at the table
header. Hall acceptance is therefore still incomplete, despite positive route
and restoration checks. The next diagnostic run records acceptance and original
file-call results to distinguish a UI-return problem from a write failure.

Original `gameplay-assisted-level5-1` positively completes the carpet exit,
NIS selector 1 natural return and level-six entry (room 27, step 1332).
Position/health assistance and two failed closed-gate carpet attempts are
retained in its recorded route; no level-exit result was written.
`manifest/gameplay-assisted-level5.sha256` pins the completed discovery.
Fresh `gameplay-assisted-level5-repeat-1` matches all 136 state/pixel
checkpoints and reaches the same level-six endpoint. Native pairing remains
pending.

The Hall diagnostic `.run-V5Barc` confirms accepted name entry followed by
Create/GetFInfo success and SetFInfo error -43; no file write follows. Existing
original `files-save-3` captures preserve the named `:Data:Prince.hof` and
`:Data:Fixture Save` paths across GetFInfo. These inputs are now pinned in
`manifest/gameplay-hall-path.sha256`. The native adapter preserves named
query input paths while retaining indexed name output; the ending/Hall replay
is in progress. Alone in the Dark has a matching code pattern recorded in
its FILES.1 follow-up.

Native `.run-f0UnbL` passes all 81 ending checkpoints, name entry, original
62-byte Hall write and persisted name/score comparison. The named GetFInfo
path fix is exercised through successful Create/GetFInfo/SetFInfo/Open/Write/Close;
code, input, heap and hardware restoration pass. Separate show/dismiss and
fresh-session readback now pass in the Hall-view fixture described below.

The reviewed installed launcher helper adds only an A3000 preset; its A1200-030
branch is unchanged. The installed bytes are hash-pinned in the scaffold manifest;
the unrelated shared working-tree changes were not modified.

Original `gameplay-assisted-level6-1` uses logged position assistance to reach
the two-door exit room, activates its original switch and enters level seven
(room 32) at step 280. One head-attack death/restart is retained. The completed
route is pinned in `manifest/gameplay-assisted-level6.sha256`. Fresh
`gameplay-assisted-level6-repeat-1` matches all 42 checkpoints, including the
original level-seven entry. Native pairing remains pending.

Native level-five discovery replay `.run-wHktzN` matches all 37 captured
checkpoints (pixels, palette, actor/RNG state and captured context) before stopping
at the key-step guard after frame 523. The original failed carpet attempt
restarts at step 522; the native run has not reached that restart at the
recorded boundary. This is a failed replay, not level-five completion.
A fresh assisted success route omitting the discarded failed attempts is being
captured; the existing independent death/restart gates remain required.

Fresh original `gameplay-assisted-level5-clean-1` follows the successful
switch/carpet sequence without the discarded deaths: 192 frames, 194 key reads,
46 checkpoints and 17 assisted word writes. It naturally enters level six
after its exit decision. This short route consumes that transition with copy
protection selector 100; it does not exercise carpet NIS 1. The separate
`scenes-carpet-full` route sets the logged D6 protection-accepted word before
the exit and positively returns from NIS 1 with two CopyNIS calls, then enters
level six. `manifest/scenes-carpet-reference.sha256` pins that profiling route.
`manifest/gameplay-assisted-level5-clean.sha256`
pins this shorter route. Fresh `gameplay-assisted-level5-clean-repeat-1`
matches every recorded checkpoint. Native replay is in progress.

Native level-six replay `.run-uJ8sbp` completes all 280 frames and naturally
enters level seven. The final checkpoint matches, and code/input/heap/hardware
restoration passes. Intermediate comparisons fail: checkpoint 16 pixels and
actor state, checkpoints 17–23 actor state, and checkpoints 24–40 captured context.
These failures remain unresolved and are not accepted under the existing D11
exception without evidence identifying their cause. This is endpoint coverage,
not a passing paired route.

Original `gameplay-assisted-level7-1` positively reaches level eight, room 16,
at step 905. The recorded assistance opens the room-21 exit through the
room-eight plate; failed earlier navigation remains in this discovery. After
the stairs, the original already requests level eight but waits for audio
completion before changing levels (CODE 2+6134–6168). An additional 40 frames
reaches the natural endpoint. `manifest/gameplay-assisted-level7.sha256` pins
the completed discovery. A fresh route omitting discarded attempts is being
captured; this is not yet a repeatable original/native pair.

Native `.run-4DrtS4` passes the clean level-five route: all 46 checkpoints,
192 frames and 194 key reads match, with no deferred RNG differences. NIS 1
returns naturally and level six begins. Code, input, heap and hardware
restoration pass. The earlier failed discovery replay remains recorded.

Original `gameplay-assisted-level7-clean-1` reaches level eight at step 359
using the successful plate/stairs sequence, without discarded navigation.
`manifest/gameplay-assisted-level7-clean.sha256` pins the completed route;
fresh repeat and native comparison remain pending.

Fresh `gameplay-assisted-level7-clean-repeat-1` matches every recorded
checkpoint of the clean level-seven route. Its native build is prepared.

Original `gameplay-assisted-level6-clean-2` follows the previously successful
plate/door movements from fresh startup, omitting the failed head-attack
attempt. It naturally reaches level seven at step 177.
`manifest/gameplay-assisted-level6-clean.sha256` pins this route; a fresh repeat
and native comparison remain required. The unsuccessful shortened attempt
`gameplay-assisted-level6-clean-1` is not used as acceptance evidence.

Native level-four `.run-rHrTgJ` stops at frame 1468 with
`AUDIO / INSTRUMENT NOT LOADED`, after capturing checkpoint 81. Hardware,
heap and code/input restoration pass, but the route does not complete.
The original audio-driver trace for this route is being captured to determine
the missing instrument-loading behavior; the guard remains active.

Fresh `gameplay-assisted-level6-clean-repeat-1` matches every checkpoint of
the clean level-six route. Native comparison remains pending.

A separate partial check of level-four `.run-rHrTgJ` compares its 82 captured
checkpoints with the original. It finds RNG, pixel and captured-context differences
before the audio stop; these are retained as failures, not claimed as exact
matches or automatically covered by D11. The original audio trace is the next
step toward resolving the loud stop.

Original `gameplay-assisted-level4-audio-1` repeats all route checkpoints and
records a note for unloaded instrument 32 at step 1467. Its discovery endpoint
is present, but the launcher rejects the missing audio-observer completion
signal. The discovery finish path now finalizes the observer; a fresh trace
is running. The failed launcher result is not reported as a pass.

The isolated original `audio-unloaded-note-1` fixture confirms a successful
no-voice return for an unloaded instrument, with loaded/reloaded positive
controls. The host regression fails on the old guard and passes with the
correction. Native level-four regression remains pending.

Native level-seven `.run-1953oB` reaches a new first-frame boundary at step 357
where the original transitions at step 359. Its strict first-frame guard stops
the replay; this is not a passing pair.

Fresh `gameplay-assisted-level4-audio-2` passes the launcher completion check
and every original route checkpoint. Its pinned trace independently confirms
the unloaded instrument-32 request, zero return and no allocated voice.
`check_audio_unloaded_note.py --route` verifies that live request alongside the
isolated fixture. Native `.run-ZvgynP` has passed the former frame-1468 stop;
its final route result remains pending.

Native `.run-ZvgynP` passes the unloaded-instrument request and reaches the
new first-frame boundary at step 1888; its actual level/room was not captured.
The original enters level five at 1889,
so the strict first-frame guard rejects the replay. Hardware, heap and input/code
restoration pass. The audio blocker is resolved; the route's timing and
intermediate differences are explained by the exit and timer/RNG fixtures below.

The live level-eight discovery had not picked up the sword at checkpoint 101:
the capture still shows it on the floor. Positioning at x=480 and pressing Shift
then enters original NIS selector 6, which returns naturally. Checkpoint 104
shows the sword removed. Earlier NIS 4/7 returns in the same long discovery
are not evidence of sword pickup. The discovery subsequently lands on the
room-23 plate, opens its left exit and naturally reaches level nine, room 10,
at step 2917. `manifest/gameplay-assisted-level8.sha256` pins the completed
discovery, including the discarded attempts. A clean fresh route and native
pair remain required.

Native clean level-six replay `.run-uuUK13` reaches level seven at step 177
with all 28 checkpoint counters, pixels, palette, actors and RNG matching.
Hardware, heap, code and input restoration pass. Full-context comparisons fail
at ordinals 10–26: the same differences persist within offsets 2008–2027.
Their meaning remains unverified; this is not yet a passing full comparison.

The opt-in route probe now preserves the actual first-frame counters and
level/room before checking the strict expected boundary. Diagnostic native
`.run-oqFGdv` confirms level-seven replay enters level eight, room 16, at
step 357, with 359 key reads. The original enters at step 359. This establishes
the endpoint but does not pass the timing comparison; the guard is unchanged.

MEASURED: diagnostic `.run-vyUFKZ` matches all 23 preceding checkpoints and
the rejected level-eight entry in viewport pixels, displayed palette, full
context, actors and RNG. The destination is room 16; native counts remain
357 frames/359 key reads versus original 359/361.
`tools/inspect_gameplay_rejected_entry.py` records `route_accepted: false`;
`manifest/gameplay-level7-rejected-entry.sha256` pins the evidence.

Fresh original `gameplay-assisted-level8-clean-2` completes the successful
sword/plate/stairs route in 490 frames and enters level nine, room 10.
`manifest/gameplay-assisted-level8-clean.sha256` pins the completed route.
A fresh original repeat and native comparison remain required. The earlier
clean-1 preparation failed before emulator launch and is not route evidence.

Fresh `gameplay-assisted-level8-clean-repeat-1` matches every checkpoint of
the pinned clean level-eight route and passes the reference launcher.

Original `gameplay-assisted-level9-1` uses logged room/position/health
assistance, then the original running-jump exit decision to reach level ten,
room 22, at step 37. `manifest/gameplay-assisted-level9.sha256` pins this
completed route. No horse cinematic enter/return appears in its log; this
is level-exit evidence only. Fresh repeat and native pairing remain open.

Fresh `gameplay-assisted-level9-repeat-1` matches every recorded checkpoint
and passes the original launcher. Native pairing remains pending.

Native level-four diagnostic `.run-0c0hOQ` completes all 93 checkpoints and
enters level five, room 1, at step 1889 with 1891 key reads, matching the
original boundary. Code/input, heap and hardware restoration pass. Comparison
still fails RNG, pixels, context and later actor records. The earlier frame-1888
run remains a failure; this run does not establish stable timing or a passing
paired route. No new blanket D11 exception is applied.

MEASURED: in `.run-0c0hOQ`, the first RNG mismatch is checkpoint 13
(step 146, room 9). The first context/pixel mismatch is checkpoint 22
(step 310, room 13), initially at context byte 0x0D89. Actor records match
through checkpoint 86; the final six checkpoints differ, beginning at step
1673 in room 27. The timer/RNG fixtures below establish their gameplay
significance; these actor records are not treated as rendering scratch.

MEASURED: the complete 62-byte player record at A5 minus 0x50D2 matches at
all 93 level-four checkpoints, including entry to level five. The late actor
differences are confined to zero-based slot 3 at checkpoint 87 (word +24),
and slot 4 at checkpoints 87–92 (words +10, +36, +38 and +40). The existing
CODE 4+3272 mapping identifies the player record. Matching the player does
not establish equivalence of the other actors or relax the full-state gate.

MEASURED causal control: `item16-actor3-seed-1` repeats the original level-three
prefix through checkpoint 133 (step 1865), where all actor records still match
native `.run-Vz4heJ`. Replacing only the four-byte RNG seed with that captured
native seed makes checkpoint 134 (step 1877) match all six native actor records,
RNG and route counters. The unchanged original fails the actor comparison.
This includes slot 0's position and animation decision: the effect is gameplay
state, not merely rendering scratch. Earlier torch context remains different,
so captured context and frame comparisons still fail. The diagnostic does not
accept the route or change release gameplay.
`manifest/gameplay-item16-actor3-seed.sha256` pins the completed original run;
`tools/replay_gameplay_rng_fixture.py --check-only` rechecks its unchanged prefix,
single seed write, endpoint and unchanged-reference control.

MEASURED failed control: `item16-actor4-seed-1` repeats the original assisted
level-four prefix through checkpoint 83 (step 1613), then makes the analogous
seed-only replacement from native checkpoint 86. It positively finishes at
original checkpoint 84 (step 1673), but the actor and RNG comparisons against
native checkpoint 87 fail again. Level, room and step match. The expected
native final seed occurs one RNG call before the fixture's final seed; this
does not identify which intervening call first diverged. No actor equivalence
is inferred from this test. `manifest/gameplay-item16-actor4-seed.sha256` pins
the failed control, and its check-only replay deliberately rejects it.

The completed control `item16-actor4-context-1` also aligns the five differing
context bytes at step 1613, in addition to the RNG seed. Its post-write dumps
verify exactly those changes and no actor-record writes. It reaches the next
checkpoint, but produces the same actor and RNG mismatch as the seed-only
control; two context bytes still differ. Matching that preceding seed and
context is therefore insufficient to explain the older native result.
`manifest/gameplay-item16-actor4-context.sha256` retains this failed control.

MEASURED: `item16-level4-opponents-1` repeats the canonical original route;
native `.run-KkYYZ2` repeats the older actor/RNG/context endpoints and retains
the strict 1888-versus-1889 entry failure. Both observers capture the additional
32-room opponent table. Its first difference is at step 1319: room 27, slot 0,
timer word +30 is 8 on the original and 23 on native. At step 1613, the only
five differing table words are timers: room 27 slot 0 (1/0), and room 28 slots
0 (5/7), 1 (14/27), 2 (34/12), and 4 (23/29), original/native respectively.
The first non-timer record differences appear at step 1673. These observations
explain why the earlier snapshots were insufficient; they do not yet prove
that the timer differences cause the later actor result; the timer fixture
below supplies that causal comparison.
`tools/check_gameplay_opponent_state.py` verifies the pinned captures, unchanged
original replay, strict native failure and two malformed-capture controls.
The two `manifest/gameplay-item16-opponents-*.sha256` files pin this evidence.

MEASURED causal control: `item16-actor4-timers-2` repeats the original through
step 1613, including all newly captured opponent records, then aligns the seed,
the five context bytes and exactly the five differing timer words. It never
writes the active actor records. At step 1673, all six actor records, RNG,
level/room/step, pixels, planes, palette, captured context and the entire
persistent opponent table match native. The seed/context-only control above
fails at this same endpoint. This establishes the missing timers' role in the
late actor difference. The RNG-trace analysis below identifies the preceding
ambient caller shift and timer draws; neither accepts the whole strict route. The fixture audits its exact write extent and
positive endpoint; `manifest/gameplay-item16-actor4-timers.sha256` pins it.
The initial `item16-actor4-timers-1` preparation failed at host HFS permissions
before launching the game and provides no gameplay evidence.

MEASURED: the retained opponent observers also contain complete level-four
RNG traces. All 2,952 native draws have the same seed/result sequence as the
original prefix. The first 109 calls also match frame and mapped caller.
At step 67 the next draw goes to original `PlayAmbient` but native
`CaveGetTorchFrame`; native's ambient draw occurs later. At step 1290,
`SkelDoneShaking` receives raw draws 52257/43250, original/native.
DERIVED from CODE 4+1B62..1B78: `Random(30)+15` produces timers 37/20;
the positive persistent timer is decremented at +1704. Native resets at step
1311 with raw draw 23607, producing 31. At step 1319 these calculations give
exactly the captured values 8/23. `check_gameplay_opponent_state.py` now checks
the mapped callers, entire common draw sequence, timer calculations and two
additional corrupted-trace controls. Together with the successful original
timer fixture, this connects the late actor difference to RNG-dependent
persistent state. Under D18 this is a gameplay-relevant timing consequence,
not a reason to normalize the actor records or alter the original decisions.

MEASURED: original `item16-exit4-1` repeats every canonical level-four
checkpoint and consumed key map with exit-branch observations enabled. Both
it and the retained opponent run `.run-KkYYZ2` begin waiting at step 1817.
The first sound test releases at original 1889 versus native 1888, with cue
294; both then take the original next-level branch. `check_gameplay_exit_audit.py 4` verifies the fresh original repeat, the existing pinned opponent/RNG evidence,
consecutive exit branches and two missing-transition controls. The new original
manifest is `gameplay-item16-exit4-original.sha256`; the native observations
remain in `gameplay-item16-opponents-native.sha256`. This identifies the
additional one-frame exit difference in the timer investigation as a sound-gate
timing consequence. It does not reclassify the strict native route as equal.

DERIVED: the active-character working record is A5−5056. `AnimChar`
(CODE 4+2C0C) reads its word +36 as the sequence cursor and +38 as the
sequence lookup key; CODE 4+3036 writes the frame at +10. The word +40 is
tested by character-control branches, for example CODE 4+15C6 and +3DD0.
Word +24 is read at CODE 4+3D48 and can change the update of word +28;
CODE 4+1CC8 sets it during another control path. These uses rule out simply
normalizing the differing words as rendering scratch. At the first retained
level-four actor mismatch, slot 4 names room 28 while the current room is 27;
that fact alone does not establish that its state is irrelevant to later play.

Original `gameplay-assisted-level10-1` activates the room-19 lower-left plate
(link 44) and enters the exit above, naturally reaching level eleven, room 5,
at step 4086. The link target is DERIVED from CODE 5 GetLinkRoom/GetLinkCell
and confirmed by the original door opening.
`manifest/gameplay-assisted-level10.sha256` pins this completed discovery with
its discarded attempts. A shorter fresh route and native pair remain required.

Original `gameplay-assisted-level11-1` activates the room-29 plate and uses
the room-24 exit, naturally entering level twelve, room 19, at step 292.
`manifest/gameplay-assisted-level11.sha256` pins the completed assisted route.
Fresh repeat and native pairing remain required.

Original `gameplay-assisted-level10-clean-1` completes the confirmed lower
plate/upper exit route in 292 frames.
`manifest/gameplay-assisted-level10-clean.sha256` pins the fresh route; its
repeat and native pair remain open.

Fresh `gameplay-assisted-level11-repeat-1` matches every recorded checkpoint
and passes the original launcher. Native pairing remains pending.

Native level-eight `.run-yYDLsT` stops in the sword cutscene at CODE 15+3900,
trap A9AB (AddResource), at step 126. This is an unresolved loud stop. Alone
in the Dark implements the service through its ResourceFiles adapter and the
shared ResourceDirectory/ResourceWriter; Vette has no matching trap handler.
An opt-in trace is measuring this original call and its subsequent WriteResource
before adapting that existing implementation.

Fresh `gameplay-assisted-level10-clean-repeat-1` matches every checkpoint
of the short level-ten route and passes the reference launcher.

Original `gameplay-assisted-level8-resource-1` repeats the complete clean
route with the explicit resource-write trace enabled. At CODE 15+3900 it adds
DSLV 6401 to the current resource file and WriteResource follows at +3906;
both return zero in D0, ResErr and MemErr. The original call at +3890 opens
`:Data:Prince2.opt`. `manifest/gameplay-resource-write.sha256` pins the trace
and commands. The native write implementation remains pending.

Native `.run-94PHSZ` passes the assisted level-nine route: all 15 checkpoints,
37 frames and 39 key reads match, with no deferred RNG exceptions. It reaches
level ten, room 22, and passes code/input, heap and hardware restoration.

Original `gameplay-assisted-level12-1` activates the room-24 plate and uses
the room-7 exit, naturally entering level thirteen, room 3, at step 292.
`manifest/gameplay-assisted-level12.sha256` pins all 24 checkpoints and the
assistance log. Fresh repeat and native pairing remain required.

Native level-ten `.run-OZOTgb` reaches level eleven, room 5, at step 290
with 292 key reads; the original boundary is step 292. The strict first-frame
guard rejects this run. Code/input and hardware/heap restoration pass, but
this does not establish a passing paired route.

MEASURED: `.run-CeIpbt` repeats that strict rejection and captures the
level-eleven entry before teardown. All 23 preceding checkpoints pass
`paired_frame` and captured-context comparison. The rejected entry matches
viewport pixels, displayed palette, captured context, actors, RNG, level and room;
its step count is 290 rather than 292. The strict completion gate remains
failed. `manifest/gameplay-level10-rejected-entry.sha256` pins this evidence.

`tools/inspect_gameplay_rejected_entry.py NATIVE REFERENCE --manifest MANIFEST`
reproduces the rejected-entry comparison for the assisted level-six and
level-ten routes. It verifies build identity, A1200-030 configuration,
restoration, every preceding checkpoint and the captured next-level state.
Its JSON report always records `route_accepted: false`; it does not replace
the strict completion checker. Controls reject a missing entry capture,
altered final context and a false successful-route marker.

Fresh `gameplay-assisted-level12-repeat-1` matches every recorded checkpoint
of the pinned level-twelve route and passes the original launcher. Native
pairing remains pending.

Native level-eleven `.run-GYU6Ag` reaches level twelve, room 19, at step
290 with 292 key reads, versus original step 292. The strict first-frame
guard rejects the run; code/input and hardware/heap restoration pass.
A diagnostic comparison of the 23 captured pre-stop checkpoints in each of
`.run-OZOTgb` and `.run-GYU6Ag` passes `paired_frame` and captured context
comparison. The missing final checkpoint and early boundary remain failures;
this diagnostic is not route acceptance.

MEASURED: level-eleven diagnostic `.run-sm9wiN` matches all 23 preceding
checkpoints and the rejected level-twelve entry in viewport pixels, displayed
palette, captured context, actors and RNG. The destination is room 19; the native
counts remain 290 frames/292 key reads versus the original 292/294.
`tools/inspect_gameplay_rejected_entry.py` confirms those comparisons while
recording `route_accepted: false`. The strict completion gate remains failed.
`manifest/gameplay-level11-rejected-entry.sha256` pins this evidence.

Native level-twelve `.run-bAamYp` reaches level thirteen, room 3, at
step 290 with 292 key reads, versus original step 292. The strict first-frame
guard rejects the run. All 23 captured pre-stop checkpoints pass the diagnostic
`paired_frame` and captured-context comparison. Code/input and hardware/heap
restoration pass.

MEASURED: level-twelve diagnostic `.run-XYaGwq` also captures the rejected
level-thirteen entry. All 23 preceding checkpoints and the endpoint match in
viewport pixels, displayed palette, captured context, actors and RNG. The destination
is room 3; native counts are 290 frames/292 key reads versus original 292/294.
`tools/inspect_gameplay_rejected_entry.py` records `route_accepted: false`; the
strict completion gate remains failed.
`manifest/gameplay-level12-rejected-entry.sha256` pins this evidence.

Original `gameplay-assisted-level10-audio-1` completes a fresh repeat of the
short level-ten route with the opt-in audio observer enabled. Every route
checkpoint matches, and the observer and reference launcher complete.
`manifest/gameplay-level10-audio.sha256` pins its trace and commands.
The exit interval contains driver clock queries (selector 15); this is evidence
for the timing investigation, not a proven cause or a native acceptance result.

MEASURED: native `.run-epGaak` records 2,284 audio trace rows, including
1,829 clock queries (selector 15). Each query returns the recorded engine
clock. The opt-in `POP2_ROUTE_AUDIO_TRACE` observer records a zero argument
slot for this argument-free selector rather than dereferencing an argument
packet. `manifest/gameplay-level10-native-audio.sha256` pins the trace and
run identity. The route still fails its early-entry counter check and restores
hardware; these samples do not establish the cause of that timing difference.
Ordinary release excludes the observer and passes the release audit.

The stopped original `gameplay-assisted-level8-resource-1` disk contains
DSLV 6400 (176,668 bytes) and the generated DSLV 6401 (373,788 bytes) in
`Prince2.opt`. The exported forks and MacBinary wrapper are pinned in
`manifest/resource-write-output.sha256` and retained in ignored
`tmp/resource-write-original`. `tools/check_resource_write.py` compares names,
attributes and initialized payloads rather than requiring identical resource-map
packing. Its controls reject an unchanged original file and a corrupted cache.
This exported oracle does not itself establish native write or fresh-boot load
acceptance.

Native `.run-dyQmd3` publishes both cache resources and passes that comparison.
The generated cache differs only at bytes 12..19. DERIVED: `MakeDisolveData`
(CODE 15+2D7A) leaves those timing fields uninitialized; `InitDissolve` saves the
cache at +2D0E before calling `SetDisolveDelays` at +2D1C. The latter writes both
fields at +3C4A/+3C54 after three calibration passes. The checker excludes only
those eight pre-calibration bytes of DSLV 6401; controls reject changes immediately
outside that range, missing publication and corrupted drawing data. DSLV 6400
remains compared in full.

Fresh boot `.run-ZTGAT7` loads DSLV 6401 through the resource-service probe;
all 373,788 bytes match the native-written payload, including the timing fields.
`manifest/resource-write-native-observation.sha256` pins this bounded evidence.
The dedicated `--readback` check passes resource metadata, complete payload,
A1200-030 configuration, fixture completion and restoration. Its controls reject
missing completion, changed payload/metadata, an unfinished shell and the wrong
CPU. The generic core launcher check lacks `CORE_REFERENCE`; this dedicated
result establishes only the resource fixture, not full core or gameplay acceptance. That earlier gameplay run stops at CODE 15+39A4,
`DISPLAY / PIXMAP OWNERSHIP`, after publication; the corrected run below
supersedes that failure.

Native `.run-IRSLhX`, using the shared heap-owned writer layout, passes the
former AddResource and graphics-ownership stops and reaches level nine, room 10,
at step 489 (491 key reads). Original `gameplay-assisted-level8-clean-2` reaches
that boundary at step 490 (492 reads); the strict first-frame guard therefore
rejects this route. All 48 captured pre-stop checkpoints pass `paired_frame`.
Full context differs at checkpoints 11..47, with differing bytes within
context offsets 0x2008..0x2027; their meaning is not yet established. Code/input
and hardware/heap restoration pass. The published cache passes the original
initialized-payload comparison. `manifest/gameplay-level8-native-write.sha256`
pins this partial evidence; it does not establish full route acceptance.

Original `gameplay-assisted-level13-flame-1` completes the altar death, automatic
shadow/flame sequence and exit through room 32, reaching level fourteen, room 1,
at step 1517 with 1519 key reads. The altar route uses logged health and position
assistance; the original guard kills the Prince, and the original shadow sequence
revives him with special state 13. The later moves and unsuccessful door attempts
remain in the route log. `manifest/gameplay-assisted-level13.sha256` pins all
37 checkpoints and interventions. This is positive original exit evidence;
native acceptance remains incomplete.

Fresh `gameplay-assisted-level13-repeat-1` completes the same level-thirteen
route and passes the reference launcher. All 37 checkpoints, including
pre-assistance captures, match pixels, palette, captured context and the curated
A5 actor/RNG/counter spans.

Native `.run-t1THYa` traverses the altar/flame/exit route and enters level
fourteen, room 1, at step 1515 with 1517 key reads, two steps earlier than the
original. The strict first-frame guard rejects that boundary. Of 36 retained
captures, checkpoints 0..13 pass the paired-frame and captured-context comparisons;
pixel/context differences start at checkpoint 14, and checkpoint 25 also
differs in RNG. These differences have not been classified as the D11 exception.
The final expected checkpoint is absent. Code/input and hardware/heap restoration
pass. `manifest/gameplay-level13-native.sha256` pins this partial evidence,
not full route acceptance.

MEASURED: closer comparison of those 36 retained captures finds identical
actor records throughout. The only differing captured-context byte is 0x0915
at checkpoints 14–35. Pixel differences occur at checkpoints 14–24; the first
is confined to viewport x44–85/y134–205, visually the blue altar flame.
Later checkpoints 25–35 have identical viewport pixels. This localizes the
remaining state/pixel difference but does not establish its cause or extend
the D11 exception; the containing route remains rejected.

MEASURED: fresh original `item16-flame-1` repeats the complete canonical route.
Its read-only CODE 10+2CC2/+2D48 observations pair 613 consecutive updates at
steps 28–640. The original update increments a twelve-state cycle or decrements
the bit-7 shrinking state; the only intervening reset changes state 9 to 133
at step 229. Current native `.run-7TG1jl` executes the same 613 updates and
recurrence, but resets state 8 to 133 at step 228. The subsequent animation
phase differs. CODE 10+29F8 and +2AB4 contain the original writes of 133 to
context word 0x0914; the event and sound-gate pairs below establish the
changed reset timing.

MEASURED: `item16-flame-events-1` repeats the full original route while
capturing steps 160–240 and both possible reset sites. Its only reset is
CODE 10+29F8 at step 228, when the active shadow character reaches frame 154;
the altar-transition site +2AB4 does not execute in this observed interval.
The next flame update at step 229 sees state 133. This locates the event in
shadow animation, not in the flame update recurrence. Native `.run-jFpn9K`, with the same executable as `.run-7TG1jl`, resets at
step 227 and retains the rejected level-fourteen entry at step 1515. Only
shadow slot 1 differs at each same-step comparison from 160 through 240.
All six native actor records at steps 160–239 equal the original records one
step later: 80 consecutive exact pairs. This explains the flame phase shift
as an earlier shadow animation; the initial divergence lies before step 160.
`tools/check_gameplay_flame_events.py` verifies the complete original repeat,
81 consecutive captures, both reset publications, native restoration, the
shifted actor pairs and two corruption controls. The two
`manifest/gameplay-item16-flame-events-*.sha256` files pin the evidence.

DERIVED: CODE 4+3C60/+3C72 waits for both current death-sound cues via
`PrinceSndOn`; after completion, +3C96 writes 600 to A5−4FCC. `FrameAdv`
sets A5−4FD4 at CODE 2+685E when that state is present in the altar room.
CODE 10+2B08 checks this flag and +2B14 creates the shadow. This identifies
the sound gate preceding the shadow sequence; the paired observations below
measure the initial timing difference between steps 40 and 160.

MEASURED: original `item16-shadow-gate-1` repeats all 28 canonical route
captures. It waits for the second death-sound cue (188) from step 47 through
149, commits the completed death state at 149, then creates the shadow at
150. Native `.run-NeBYwx` begins that same wait at 47 but completes it at 148
and creates the shadow at 149. Both execute the original branch sequence.
All 36 preceding native captures and the rejected entry retain the previous
flame-event run's pixels, captured context, actors, RNG and frame counters;
the executable is identical. `check_gameplay_shadow_gate.py` verifies the
complete consecutive sound checks, shadow creation, original repeat, native
restoration/build identity, baseline captures and two corruption controls.
The two shadow-gate manifests pin this pair. This closes the chain from
asynchronous death-sound completion through the one-frame shadow lead to
the flame reset. D18 accepts that timing consequence without changing game
decisions or reclassifying the strict route as an exact match.
The earlier native `.run-lHCZN5` correctly failed its byte guard before any
shadow observation because the observer armed before CODE 10 was loaded;
the passing observer uses the established level-thirteen arming checkpoint.

`tools/check_gameplay_flame_window.py` checks these pinned observations,
the original repeat, native build/restoration audits and two corruption
controls. All 36 preceding native actor records, RNG values and route counters
match the original in this fresh run. Context differs only at byte 0x0915 in
ordinals 14–35; viewport pixels differ in ordinals 14–24, with matching palettes.
This run does not reproduce the older preceding RNG difference, and its flame
phase differs in the opposite direction. The rejected entry still occurs at
1515 rather than 1517 and has a different RNG seed. Both historical failures
remain retained; no full-route equivalence is claimed. Evidence is pinned in
`manifest/gameplay-item16-flame-original.sha256` and
`manifest/gameplay-item16-flame-native.sha256`.

MEASURED: `.run-jXfO5v` reproduces the altar differences and captures the
rejected level-fourteen entry in room 1. Endpoint viewport pixels, displayed
palette, captured context and actor records match; RNG does not. Native counts are
1515 frames/1517 key reads versus original 1517/1519. Code/input audits and
hardware/heap restoration pass. `manifest/gameplay-level13-rejected-entry.sha256`
pins the final capture and diagnostic report, which explicitly retains
`route_accepted: false`. The sound/shadow/flame chain and terminal RNG
observations above explain these differences under D18.

Original fresh-session `gameplay-hall-view-4` imports the pinned Hall file
written by `gameplay-assisted-hall-4`. Command-H runs `ShowHallOfFame`
(CODE 2+28C2); its loaded table and visible display contain `codex`, score 75.
After the original fade-in returns, Escape dismisses the table. The original
pause loop is then reached; Space resumes gameplay, observed at FrameAdv return.
No original control flow is bypassed. `manifest/gameplay-hall-view-reference.sha256`
pins the imported file, loaded table, visible/returned captures and completion log.
`tools/check_gameplay_hall_view.py` checks these positive lifecycle endpoints and
six altered-input/completion controls reject. Fresh `gameplay-hall-view-repeat-1`
passes the same lifecycle endpoints, with an identical loaded table and complete
visible Hall image.

Native `.run-XGr0WK` passes the corresponding fresh-session Hall lifecycle on
`a1200-030`. The imported data/resource forks and Finder metadata come from the
original-written MacBinary wrapper, decoded by the existing save importer;
`manifest/gameplay-hall-import.sha256` pins that input. The loaded table is
byte-identical to the original, and the D8-owned display visibly shows `codex`
and 75. Original fade-in, ShowHallOfFame return, pause entry and completed
FrameAdv are observed in order. Native diagnostic code supplies Escape and Space
through the existing VBI input harness and stops only after the resumed frame
has been published. GDB observes without writing cached runtime variables.
Code/input and heap/hardware restoration pass; seven altered native evidence
controls reject. `manifest/gameplay-hall-view-native.sha256` pins this evidence.
This closes the separate show/dismiss and fresh-session readback coverage;
the ending/name-entry/write pair remains independently recorded above.

Reproduce the native fixture using the pinned original Hall references:

```sh
. amiga/env.sh
make -C amiga VERIFY=1 EXTRA_DEFINES='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_GAMEPLAY_RESTART_PROBE -DPOP2_GAMEPLAY_HALL_VIEW_PROBE'
CORE=1 GAMEPLAY_HALL_VIEW_REFERENCE="$PWD/ref/mame/runs/gameplay-hall-view-4" \
  GAMEPLAY_HALL_IMPORT_REFERENCE="$PWD/ref/mame/runs/gameplay-assisted-hall-4" \
  GDBSCRIPT=amiga/gameplay_hall_view.gdb AMIGA_SETUP=a1200-030 SOUND=0 \
  DEADLINE=900 bash amiga/run.sh
make release
```

The launcher checks the positive endpoint, imported/table bytes, build identity
and effective machine. Add `--controls` to the standalone Hall-view checker
with `--native RUN` to exercise evidence rejection. All new input sequencing
and completion probes require explicit verification flags and are excluded from
ordinary release builds.

### Timed-story discovery limit

MEASURED: `gameplay-timed-stories-1` used logged remaining-minute assistance
and forward level-four-to-five transitions to reach actual selectors 12, 13
and 14. Selectors 12 and 13 returned normally; selector 14 reached its fade-in
checkpoint but had no return before the original reference's execution limit.
The reference ended with `GAMEPLAY ROUTE INCOMPLETE`, and the launcher rejected
it. This run is diagnostic only, not accepted story coverage or a native failure.
The attempted timer assistance remains unvalidated for the complete story route.

MEASURED: the isolated `gameplay-story14-isolated-1` route and fresh
`gameplay-story14-isolated-repeat-1` both complete actual story selector 14,
with normal return and explicit discovery completion. All route checkpoints
repeat, and all nine selector-14 capture files are byte-identical. The references
are pinned in `manifest/gameplay-story14-reference.sha256` and
`manifest/gameplay-story14-repeat.sha256`. This does not resolve the combined-route
execution limit above.

The opt-in `assist minutes VALUE` command accepts 0–75 remaining minutes for
a live actor. It logs the minute write and resets the two subminute words and
timer redraw flag, following the original minute-cheat fields. The assisted
input checker verifies the complete before/after A5 captures and unchanged
context and pixels. This route uses 46 minutes and first runs story selector 12
before a further forward level transition selects 14. Four input-checker tests
pass, including the pinned timer writes and rejection of out-of-range values.

MEASURED: native `.run-LunkbY` completes the isolated story-14 route. Its
fade-in, copy and normal-return captures match pixels, displayed palette and
curated game state; all 21 containing route checkpoints match with no deferred
exceptions. `tools/check_gameplay_story.py` checks both scopes, and
`manifest/gameplay-story14-native.sha256` pins the evidence. Controls reject
missing completion/return markers, an incorrect final result, an altered pixel
and altered RNG state. The route only loads level entries and completes no
FrameAdv updates, so its last-result diagnostic retains the zero initializer;
the route checker now requires zero in that specific case, retaining -2 for
routes with completed updates. The initial native launcher result was a false rejection
of that counter; the standalone comparison passes with the corrected check.

MEASURED: `gameplay-story15-isolated-1` repeats the isolated story sequence
with 37 remaining minutes and reaches actual selector 15. The fresh
`gameplay-story15-isolated-repeat-1` matches all route checkpoints and all nine
cinematic capture files. Both record normal story return and discovery
completion. `manifest/gameplay-story15-reference.sha256` and
`manifest/gameplay-story15-repeat.sha256` pin the references.

MEASURED: native `.run-WeoTKt` completes story 15 and its assisted route.
`tools/check_gameplay_story.py --selector 15` passes all three cinematic
checkpoints and all 21 route checkpoints, with normal return, hardware
restoration and no deferred differences. The native launcher also passes the
route comparison. `manifest/gameplay-story15-native.sha256` pins the evidence.

MEASURED: `gameplay-story16-isolated-1` uses 28 remaining minutes to select
actual story 16. Fresh `gameplay-story16-isolated-repeat-1` matches every route
checkpoint and all nine cinematic capture files, with normal return and
discovery completion in both sessions. The original evidence is pinned in
`manifest/gameplay-story16-reference.sha256` and
`manifest/gameplay-story16-repeat.sha256`.

MEASURED: native `.run-6qIcjr` completes story 16 and the containing assisted
route. The story checker passes all three cinematic checkpoints and all 21
route checkpoints, with normal return, restoration and no deferred differences.
The launcher also passes the route comparison.
`manifest/gameplay-story16-native.sha256` pins the native evidence.

MEASURED: `gameplay-story17-isolated-1` selects actual story 17 with 19
remaining minutes. Fresh `gameplay-story17-isolated-repeat-1` matches every
route checkpoint and all nine cinematic capture files. Both sessions record
normal story return and discovery completion. The original evidence is pinned
in `manifest/gameplay-story17-reference.sha256` and
`manifest/gameplay-story17-repeat.sha256`.

MEASURED: native `.run-UuAc30` completes story 17 and its assisted route.
The story checker passes all three cinematic checkpoints and all 21 route
checkpoints, with normal return, restoration and no deferred differences.
The launcher also passes the route comparison.
`manifest/gameplay-story17-native.sha256` pins the native evidence.

MEASURED: `gameplay-story18-isolated-1` selects actual story 18 with 10
remaining minutes. Fresh `gameplay-story18-isolated-repeat-1` matches every
route checkpoint and all nine cinematic capture files. Both sessions record
normal story return and discovery completion. The original evidence is pinned
in `manifest/gameplay-story18-reference.sha256` and
`manifest/gameplay-story18-repeat.sha256`.

MEASURED: native `.run-DfHyCq` completes story 18 and its assisted route.
The story checker passes all three cinematic checkpoints and all 21 route
checkpoints, with normal return, restoration and no deferred differences.
The launcher also passes the route comparison.
`manifest/gameplay-story18-native.sha256` pins the native evidence.

MEASURED: `gameplay-story19-isolated-1` selects actual story 19 with one
remaining minute. Fresh `gameplay-story19-isolated-repeat-1` matches every
route checkpoint and all nine cinematic capture files. Both sessions record
normal story return and discovery completion. The original evidence is pinned
in `manifest/gameplay-story19-reference.sha256` and
`manifest/gameplay-story19-repeat.sha256`.

MEASURED: native `.run-F5MwKY` completes story 19 and its assisted route.
The story checker passes all three cinematic checkpoints and all 21 route
checkpoints, with normal return, restoration and no deferred differences.
The launcher also passes the route comparison.
`manifest/gameplay-story19-native.sha256` pins the native evidence.

MEASURED: `gameplay-story13-isolated-1` selects actual story 13 with 55
remaining minutes. Fresh `gameplay-story13-isolated-repeat-1` matches every
route checkpoint and all nine cinematic capture files. Both sessions record
normal story return and discovery completion. The original evidence is pinned
in `manifest/gameplay-story13-reference.sha256` and
`manifest/gameplay-story13-repeat.sha256`.

MEASURED: native `.run-a0jjep` completes story 13 and its assisted route.
The story checker passes all three cinematic checkpoints and all 21 route
checkpoints, with normal return, restoration and no deferred differences.
The launcher also passes the route comparison.
`manifest/gameplay-story13-native.sha256` pins the native evidence.

MEASURED: `gameplay-story20-expired-3` sets zero remaining minutes through
logged assistance and follows the original expiration branch. Actual selector
20 returns 0; its PlayNIS wrapper returns 1 at CODE 2+71A6. The opt-in
`POP2_ROUTE_EXPIRED_PROBE=1` observes that return and explicitly finishes the
reference. Fresh `gameplay-story20-expired-repeat-1` matches all four route
checkpoints and all nine cinematic capture files. Both pass the launcher.
`manifest/gameplay-story20-reference.sha256` and
`manifest/gameplay-story20-repeat.sha256` pin the reference evidence.

MEASURED: native `.run-2ERlhy` matches all three cinematic checkpoints and
all four route checkpoints, with no deferred differences and successful
restoration. `manifest/gameplay-story20-native.sha256` pins the pair. The
verification-only terminal callback checks the original expiration caller,
selector 28, instruction bytes and zero-minute state before observing its
return; ordinary release does not contain this probe. Five input tests pass,
including controls rejecting missing completion/return evidence, a non-normal
wrapper result, a different cinematic selector and an incorrect final checkpoint.

The rejected original `gameplay-story20-expired-1` tested the wrong wrapper
result (0 rather than 1). `gameplay-story20-expired-2` reached the explicit
endpoint but lacked launcher recognition of its completion marker. Neither is
used as the accepted reference; the fresh run and repeat above pass the corrected
checks without changing original game execution.

MEASURED: `gameplay-level6-context-2` repeats every checkpoint of the clean
level-six reference with a read-only write tap. The captured context allocation
is checked against its physical alias before arming the tap. The differing
0x2008–0x2027 bytes are written by original `AddOppToRoom`, CODE 6+3602 and
its structure-copy loop at +3616/+361C (the tap reports the following PC).
This localizes the discrepancy to an original opponent-record copy; it does
not yet establish that any of those bytes may be excluded from comparison.
The trace reaches the same level-seven endpoint and passes the original repeat.

The more specific original repeat `gameplay-level6-context-3` also matches every
checkpoint. At step 6, `TransferOppInfo` calls `AddOppToRoom` with both room
and record index equal to signed -1, copying 38 bytes to context+0x2002.
The source record is captured before the copy. This is evidence of the original
path, not yet a justification for ignoring its differing bytes. The trace log
and source record are pinned in `manifest/gameplay-level6-context.sha256`;
`POP2_ROUTE_CONTEXT_TRACE=1` enables this read-only reference diagnostic.

MEASURED: native `.run-mdtypS` reproduces that same step-6 copy with room/index
-1 and destination context+0x2002. On both systems the captured 38-byte source
exactly equals the destination in the next route checkpoint. The native route
positively completes all 28 checkpoints and enters level seven at step 177,
with clean restoration and code/input audits. Its captured-context checker still
rejects ordinals 10–26. This localizes the mismatch to the source record rather
than a differing copy implementation; it is not a full-route pass.
`manifest/gameplay-level6-context-native.sha256` pins the diagnostic evidence.

### Level-six alternate assisted door route

MEASURED: `gameplay-level6-door-1` starts from the original level-six entry,
uses logged health and position assistance, and walks left in room 16 to
activate the door. It waits in room 22, returns to the opened door and reaches
level seven, room 32, at step 333. The reference positively finishes discovery;
`manifest/gameplay-assisted-level6-door.sha256` pins 25 checkpoints, 335 key
reads and 20 assistance writes. The same context observer that detected the
invalid opponent-record copy on the earlier route records none on this route.
Fresh `gameplay-level6-door-repeat-1` matches every recorded checkpoint and
the same level-seven endpoint. Its log/actions are pinned in
`manifest/gameplay-level6-door-repeat.sha256`. Full native comparison remains
required; the earlier context failures are not reclassified as passes.

The discarded `gameplay-level6-direct-1` and `gameplay-level6-plate-1` attempts
missed the door trigger and do not establish exit coverage. A neutral frame
following the old room teleport (`gameplay-level6-refresh-1`) did not prevent
its invalid-index copy. These controls do not replace or relax the failed
captured-context comparisons recorded above.

MEASURED: native `.run-q5NbV0` matches all 24 captured checkpoints of the
alternate door route: counters, captured context, actors, RNG, viewport pixels,
palette and copper output. It then reaches level seven, room 32, at step 332
instead of 333. The strict `GAMEPLAY ROUTE / FIRST FRAME` guard stops before
the final checkpoint, so this is not a completed paired route. The earlier
context discrepancy is absent from the captured portion; the paired exit and
queued-note observations above explain the boundary timing under D18. Native evidence is pinned in
`manifest/gameplay-level6-door-native.sha256`.

MEASURED: `.run-7yMRHs` repeats that rejection and captures the level-seven
entry before teardown. Its viewport, displayed palette, captured context,
actors, RNG, level and room match the original final checkpoint. The step
counter remains 332 instead of 333 (334 instead of 335 key reads); all 24
preceding captures pass `paired_frame` and captured-context comparison.
`manifest/gameplay-level6-rejected-entry.sha256` pins this diagnostic evidence.
The verification-only rejected-boundary callback observes the failure without
accepting it or applying further assistance. The strict route checker still
rejects completion; this localizes the remaining discrepancy to the boundary
timing among the compared fields, rather than establishing route acceptance.

### Ship cutscene reference

MEASURED: `gameplay-ship-cutscene-1` repeats all 95 level-one route checkpoints,
reaches level two at step 1001 and positively finishes discovery. Actual NIS
selector 9 returns 0 after 13 CopyNIS and eight NISTextDraw callbacks; all three
actual NIS calls are accounted for. Visible captures show the departing ship,
the sleeping prince and the storm/ lightning sequence. The return image is
black. `manifest/gameplay-ship-reference.sha256` pins those captures and the
completed route log. Native cinematic comparison remains pending.

MEASURED: native `.run-usSSPj` matches the ship fade-in, text-4, storm copy-8
and black return checkpoints, including displayed colour and curated game
state. Selector 9 returns normally with matching callback counts. The containing
level-one route passes all 96 checkpoints with only its two previously pinned
D11 RNG exceptions. `tools/check_gameplay_ship.py` checks both scopes and rejects
a corrupted visible pixel. `manifest/gameplay-ship-native.sha256` pins the native
evidence. The launcher's Bash 3.2 empty-array expansion error occurred after
normal restoration; the standalone checker supplies the passing comparison.
The launcher now handles both empty and nonempty optional argument lists.

MEASURED: alternate level-eight original `gameplay-level8-direct-room-1`
enters the throne room directly through logged assistance, retaining the throne
interaction and natural stair exit. It reaches level nine, room 10, at step 479
with 481 key reads. Fresh `gameplay-level8-direct-room-repeat-2` matches all 49
recorded checkpoints. The two `manifest/gameplay-assisted-level8-direct-room`
SHA-256 manifests pin the original and repeat. This variation still reaches
the invalid opponent-record copy at step 180 while crossing into room 24;
it does not establish a fix or native acceptance. The first repeat preparation
failed before emulator launch and is not reference evidence.

MEASURED: native `.run-R8yRkz` replays that alternate level-eight route.
All 48 captured pre-entry checkpoints pass `paired_frame`; captured context matches
through ordinal 27, then differs at 0x2008–0x2027 for ordinals 28–47 after
the room-24 crossing. Native reaches level nine, room 10, at step 478 with
480 key reads, versus original 479/481, and the strict first-frame guard rejects
completion. `manifest/gameplay-level8-direct-room-native.sha256` pins the
failed run. This variation moves the invalid copy later but does not resolve it.

MEASURED: original `gameplay-level8-direct-stairs-1` retains the throne
interaction, enters room 24 through logged assistance and then uses the stairs
to reach level nine, room 10, at step 1193 with 1195 key reads. Its 78 checkpoints
include the unsuccessful first stair approach and subsequent continuation;
no emulator restore was used. The same observer that recorded the invalid
opponent copy on the preceding route records none here.
`manifest/gameplay-assisted-level8-direct-stairs.sha256` pins this completed
original route. Fresh `gameplay-level8-direct-stairs-repeat-1` matches all 78
checkpoints and passes the reference launcher;
`manifest/gameplay-assisted-level8-direct-stairs-repeat.sha256` pins that repeat.
MEASURED: native `.run-EhJVft` matches all 77 preceding checkpoints and the
rejected level-nine entry in viewport pixels, displayed palette, captured context,
actors and RNG. The entry is room 10 at native step 1191/1193 key reads, versus
original 1193/1195. The invalid-copy context mismatch is absent on this assisted
route, but the strict timing gate remains failed.
`tools/inspect_gameplay_rejected_entry.py` records `route_accepted: false`;
`manifest/gameplay-level8-direct-stairs-native.sha256` pins this evidence.
