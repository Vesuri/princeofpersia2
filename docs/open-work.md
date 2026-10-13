# Open work

Owner-approved order: performance (13), then timing (11), gameplay validation
(12), and release (14). Items remain open until their acceptance gates pass.
Timing prerequisites already in progress support performance measurements
without completing item 11.

13. **Performance.** Owner-directed current target: `a1200-030` only. Fix its
    performance first, using Kalms C2P on dirty rectangles in one end-of-frame
    conversion pass. Profile the end-to-end build before optimising and use
    matched workloads. Defer 68020 measurements and optimisation until this is
    established. Set the Fast RAM requirement (D5); the deferred 68020 work
    remains part of the later timing gate.
    Acceptance: matched-workload measurements for each change, A/B against parent.
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
