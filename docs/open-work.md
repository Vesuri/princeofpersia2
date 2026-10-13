# Open work

Owner-approved order: gameplay validation (12), release including 68040 validation
(14), then 68020 support (15). The current validation target is `a1200-030`;
the owner deferred both 68040 and 68020 work. Items remain open until their
acceptance gates pass.

12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Acceptance: coverage matrix with positive completion records for every entry.
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
