# Open work

Owner-approved order: timing (11), gameplay validation (12), and release (14).
Items remain open until their acceptance gates pass. The completed performance
gate covers `a1200-030`; deferred 68020 measurements belong to the timing gate.

11. **Timing.** 60 Hz ticks from PAL fields, Time Manager and VBL tasks at safe
    user-mode points, completed frames paced at vertical blank. Inspect and reuse
    Alone in the Dark's queued VBL passes through the shared workflow; replace the
    local elapsed-tick collapse and consolidate reusable scheduler logic.
    Acceptance: game-step cadence matches the reference on the fixed 68020 and on
    fast CPU configurations.
12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Acceptance: coverage matrix with positive completion records for every entry.
14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, 68020 and AGA checks; 68040 MMU
    test; deterministic LHA.
    Acceptance: the playbook §14 release checklist, including real hardware.
