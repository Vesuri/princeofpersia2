# Open work

The owner brought 68020 support (15) ahead of release including 68040 validation
(14); item 19's reductions serve both. Gameplay coverage (12) remains accepted
under D10–D12 on `a1200-030`; 68040 validation remains part of release work.
Items remain open until their acceptance gates pass.

15. **68020 support.** Measure and optimize
    the fixed 68020 configuration without changing original game decisions.
    Two current baseline runs miss all 53 five-tick intervals and measure
    424–428 ms/update in the matched five-update window. Full-route death pixels
    also differ from the pinned parent. Item 19's opening profile on `a1200-fast`
    misses all 81 requested intervals (about 12 fields per frame) and shows port
    work about four times the original's. The original's own landing work is
    99.8 ms/frame, above the 83.3 ms five-tick budget, so port optimization alone
    cannot meet that scene's cadence (DERIVED). See `docs/performance.md`. This
    does not waive the acceptance gate; changing it requires an owner decision.
    Pursue item 19's reductions first.
    Acceptance: original timing-service contracts and game-step cadence pass on
    `a1200-fast`, matched gameplay regressions pass, and the release hardware and
    installer requirements accurately include the validated configuration.

14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, CPU compatibility and AGA checks;
    deferred 68040 timing-service and original game-step cadence validation,
    68040 MMU test; deterministic LHA.
    Acceptance: the original/native timing gates pass on every additional claimed
    CPU configuration, and the playbook §14 release checklist passes, including
    real hardware. The 68040 cadence overrun remains unresolved; 030 results do
    not establish 040 acceptance.

16. **Deferred gameplay timing and state differences (D11/D12).** Investigate
    the recorded one- or two-frame early exits in levels 3, 6–8 and 10–13;
    ambient-audio RNG timing; level-three wall-torch animation and final-entry
    actor slot 0; level-four animation/context, RNG and late actor slots 3/4;
    and level-thirteen altar flame/context and RNG differences. The player
    record matches the recorded level-three entry and all level-four captures;
    the gameplay relevance of the other actor differences is not established.
    Keep the earlier invalid opponent-record copies in the level-six/eight
    discovery routes as separate failed controls; the accepted alternate routes
    avoid those copies. Evidence and exact run/manifest identities are in
    `docs/gameplay-coverage.md`. Item 12's owner-authorized acceptance is not
    an exact-match result: preserve failed strict reports and named guards.
    A fresh item-19.1 integration reference (`20261015-151738-f9137e07`) also
    fails its existing A5 pin at byte A5−0B77 (F3 versus F6), outside the two
    already normalized clock words; first-room pixels, palette and context match.
    Keep the failed pin check and establish that byte's role before changing
    normalization. The 19.1 regression uses the documented pinned-reference mode.
    Acceptance: explain and resolve the differences with original-paired
    evidence, including any effects on gameplay decisions. Check the other
    ports before service changes; keep diagnostics opt-in and release ordinary.

17. **Deferred discovery-state input continuity.** Investigate why restored
    Macintosh discovery attempts can consume different input after identical
    short commands when the preceding held keys differ. Keep this separate
    from the remaining level routes; preserve the existing requirement for
    fresh original repeats without emulator restores.
    Acceptance: controlled held-key and neutral-key restore cases establish
    input continuity, or the discovery tool explicitly detects and reports
    unsupported restore conditions. Consult the other ports before changing
    shared Macintosh input helpers.

18. **Deferred freed-handle error classification.** Original System 7.5.5 returns
    `memWZErr` for the disposed palette alias at CODE 15+29C2 in story scene 20,
    but an isolated NewHandle/DisposeHandle/HUnlock fixture returns `memAZErr`.
    The current gameplay fix supports only the measured caller and a verified
    free master slot; arbitrary pointers and unmeasured invalid calls still stop.
    Investigate broader memory-layout-dependent error classification after
    gameplay coverage, consulting the other ports and shared heap workflow.
    Acceptance: paired controls establish supported cases without accepting
    unknown pointers or silently changing the free list.

19. **Scene profiling and reductions.** The opening sequence (window entry,
    guard attack, fall; `docs/performance.md#opening-sequence-profile-item-19`) is
    profiled with `amiga/aprof.sh`. It meets the original's requested cadence on
    the selected `a1200-030` preset, but the exact no-MMU 68030 model misses 10
    of 81 intervals in the landing and the 68020 misses all in the initial
    profile. That baseline has port work four to five times the original game's;
    reduction results are recorded in `docs/performance.md`. Work in order, one
    commit each. Dispatch and clock reductions pass their D13 active-work budgets
    and per-trap limits; their matched measurements and common gates are recorded
    in `docs/performance.md`. The exact no-MMU opening still has two late
    intervals. For every remaining step report the opening profile before and after
    (`opening`, `report`, `work` per phase) with two repeats, the per-step
    service gates named below, `make integration-check`, the timing cadence and
    VBL/Time Manager fixtures, and an unchanged ordinary release audit. Check
    Vette! and Alone in the Dark first; Alone in the Dark's FRAME.4, FRAME.6 and
    HEAP items solved similar costs. Figures are 68030 ms per landing frame
    unless noted.

    - **19.4 Non-blocking buffer synchronization.** `PrinceScreen::synchronize`
      spins 14.1 ms on the blitter in the landing (3.6 attack, 1.3 fall). Start
      the copy and wait only before the next write to that buffer.
      Acceptance: the four-frame display fixture and integration planes pass;
      CPU blitter waiting ≤ 2 ms per landing frame.
    - **19.5 Landing conversion bounds.** C2P takes 18.6 ms per landing frame
      (6.7 stand-off, 4.9 fall). Establish whether the dirty rectangles are the
      minimal combined Kid, glass and guard bounds; consider Alone in the Dark's
      disjoint-band conversion.
      Acceptance: converted planes equal the chunky image at every checkpoint
      and C2P falls by at least 30%, or the bounds are shown minimal.
    - **19.7 Heap allocation cost.** `NewHandle` takes 3.5 ms per call (68020:
      15.9 ms) and resize/dispose coalescing adds about 1.8 ms per landing
      frame. AmigaXDev holds Alone in the Dark's shared heap fixes, whose
      adoption here the owner deferred while work was ongoing; adopting them
      requires the owner's release of that deferral.
      Acceptance: heap host tests, the measured freed-handle and low-memory
      cases and gameplay save/restart regressions pass; `NewHandle` ≤ 0.5 ms.
    - **19.8 Further scenes.** Profile the level-five carpet
      exit and the level-eight throne dissolve (clean and generated cache) as
      recorded above, and add scenes whose frames exceed their requested delays.
    - **19.9 Share the profile report.** Move the common profile parsing and
      call-tree code of `tools/aprof_report.py` and Alone in the Dark's report
      into AmigaXDev once its tree is clean, keeping port-specific names and
      buckets local. The profiler stop command prints elapsed time without flushing
      the last sub-clock gap; retain the separately reported capture tail and
      add a stop-boundary control when sharing the parser. Integrate the published shared rectangle-accessor commit
      `b61c323` from `codex/pure-rectangle-accessors` when the AmigaXDev main
      working tree is clean; Prince of Persia 2 already pins that canonical source.
      The published `codex/dispatch-clock-helpers` branch also contains the
      clock-threshold helper (`95808a6`) and single-validation HLock (`5669890`),
      prepared and tested in the isolated shared checkout. Integrate these when
      the main working tree is clean; this port pins both sources.
      Single-validation HGetState (`030d3c1`) is also published on that branch
      and pinned here, with read-only whole-heap and invalid-handle controls.
      Pinned slot-range reuse and exact PAL deadline thresholds (`7e2c39c`)
      follow on the same branch, with reset/collision and boundary controls.
      The read-only code-index interface (`a5a7cd4`) lets native callers reuse
      those candidates while retaining index bounds and current-range checks.
      Validated slot flag-address reuse (`7c663eb`) follows on that branch;
      it rereads mutable flags and clears the mapping at arena reset.
      A bounded 32-slot flag-address cache (`cb13b74`, Amiga type fix
      `b2a5dc0`) follows; flags are
      always reread and reset invalidates every address candidate.
      The larger code-owner index (`a92067e`) avoids 256 KiB aliasing while
      retaining current range, size and index checks.
      The published `codex/audio-reuse-helpers` branch contains the bounded
      immutable PCM cache and settled Paula state (`a3dd031`, with Amiga type
      compatibility in `8ecd85f` and stopped-DMA state in `7759f0f`). This port
      pins these helpers; integrate the branch when the main working tree is clean.

    Acceptance: each step's gates and measured targets; finally the opening
    profile meets every requested interval on the exact 68030 model, with
    reproducible matched-workload results for the further scenes. Emulator
    results are not hardware performance claims.
