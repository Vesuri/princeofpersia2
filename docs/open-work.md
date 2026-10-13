# Open work

Owner-approved order: timing (11), gameplay validation (12), release (14), then
68020 support (15). Items remain open until their acceptance gates pass. The
current target is 68030 or better; the owner deferred 68020 support to the end.

11. **Timing.** 60 Hz ticks from PAL fields, Time Manager and VBL tasks at safe
    user-mode points, completed frames paced at vertical blank. Inspect and reuse
    Alone in the Dark's queued VBL passes through the shared workflow; replace the
    local elapsed-tick collapse and consolidate reusable scheduler logic.
    Acceptance: game-step cadence matches the reference on `a1200-030` and
    faster CPU configurations.
12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Acceptance: coverage matrix with positive completion records for every entry.
14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, CPU compatibility and AGA checks; 68040 MMU
    test; deterministic LHA.
    Acceptance: the playbook §14 release checklist, including real hardware.
15. **Deferred 68020 support.** After the preceding items, measure and optimize
    the fixed 68020 configuration without changing original game decisions.
    Acceptance: original timing-service contracts and game-step cadence pass on
    `a1200-fast`, matched gameplay regressions pass, and the release hardware and
    installer requirements accurately include the validated configuration.
