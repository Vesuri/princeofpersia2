# Open work

The owner requested 68020 item 15 ahead of release item 15. **68020 support.** Measure and optimize
    the fixed 68020 configuration without changing original game decisions.
    Two current baseline runs miss all 53 five-tick intervals and measure
    424–428 ms/update in the matched five-update window. Full-route death pixels
    also differ from the pinned parent. See `docs/performance.md`. Recommend
    extending item 19 profiling to 68020 before optimization; this does not
    establish that the target is impossible or waive its acceptance gate.
    Acceptance: original timing-service contracts and game-step cadence pass on
    `a1200-fast`, matched gameplay regressions pass, and the release hardware and
    installer requirements accurately include the validated configuration.

14. Gameplay coverage
(12) remains accepted under D10–D12 on `a1200-030`; 68040 validation remains
part of release work. Items remain open until their acceptance gates pass.

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

19. **Deferred scene profiling.** During gameplay validation, record suspected
    heavy scenes in `docs/performance.md` with their route/save and observation;
    do not interrupt item 12 for deep performance investigations. Later measure
    these workloads on `a1200-030`, separating release execution from debugger
    and verification overhead. Acceptance: reproducible matched-workload results
    for the recorded candidates; no slowdown claims based on elapsed harness time.
