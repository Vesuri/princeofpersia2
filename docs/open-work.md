# Open work

Owner-approved order: gameplay validation (12), release including 68040 validation
(14), then 68020 support (15). The current validation target is `a1200-030`;
the owner deferred both 68040 and 68020 work. Items remain open until their
acceptance gates pass.

12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Owner permits cheats and checkpoints to bypass traps/combat (D10). Prioritize
    loud-stop discovery and reaching completion over unassisted route practice.
    Acceptance: coverage matrix with positive completion records for every entry,
    identifying assistance and matching it between original and native runs.
    Owner-approved exception: the already recorded torch/RNG differences remain
    in item 16 and do not block this item. Require completed assisted routes,
    the ending and no unresolved loud stops; record those differences explicitly
    rather than reporting exact matches where they do not hold.
14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, CPU compatibility and AGA checks;
    deferred 68040 timing-service and original game-step cadence validation,
    68040 MMU test; deterministic LHA.
    Acceptance: the original/native timing gates pass on every additional claimed
    CPU configuration, and the playbook §14 release checklist passes, including
    real hardware. The 68040 cadence overrun remains unresolved; 030 results do
    not establish 040 acceptance.
15. **Deferred 68020 support.** After the preceding items, measure and optimize
    the fixed 68020 configuration without changing original game decisions.
    Acceptance: original timing-service contracts and game-step cadence pass on
    `a1200-fast`, matched gameplay regressions pass, and the release hardware and
    installer requirements accurately include the validated configuration.

16. **Deferred torch animation and RNG timing investigation.** Investigate the
    recorded original/native ambient-audio RNG timing differences and level-three
    torch animation mismatch after the current gameplay coverage work. Existing
    evidence is described in `docs/gameplay-coverage.md`; preserve failed
    comparisons without treating them as passes. Do not launch further deep RNG
    investigations while establishing routes through the remaining levels and ending.
    Acceptance: explain and resolve the recorded differences with original-paired
    evidence, including checks for any effect on gameplay decisions.

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
