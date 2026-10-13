# Open work

In playbook order. Each item's acceptance gate passes before the next item's work is
committed, except where an item says it runs in parallel.

11. **Timing.** 60 Hz ticks from PAL fields, Time Manager and VBL tasks at safe
    user-mode points, completed frames paced at vertical blank. Inspect and reuse
    Alone in the Dark's queued VBL passes through the shared workflow; replace the
    local elapsed-tick collapse and consolidate reusable scheduler logic.
    Acceptance: game-step cadence matches the reference on the fixed 68020 and on
    fast CPU configurations.
12. **Gameplay validation.** Fixed-seed, state-paired routes through all levels and
    cutscenes, save/load, death, restart, Hall of Fame and the ending.
    Acceptance: coverage matrix with positive completion records for every entry.
13. **Performance.** Profile the end-to-end build on `a1200-020` and the 68030
    reference configuration before optimising; set the Fast RAM requirement (D5).
    Acceptance: matched-workload measurements for each change, A/B against parent.
14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, 68020 and AGA checks; 68040 MMU
    test; deterministic LHA.
    Acceptance: the playbook §14 release checklist, including real hardware.
