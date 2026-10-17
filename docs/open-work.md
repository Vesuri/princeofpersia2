# Open work

Version 0.90 release work (14) takes priority. Under D14, 68020 support (15)
remains deferred beyond 0.90, retaining its acceptance gate. This does not waive
item 14’s 68040 cadence, real-hardware or installer validation. Gameplay coverage (12)
remains accepted under D10–D12 on `a1200-030`.
Under D15, cinematic deadline item 19.10 and diagnostic wrapper item 21 are complete.
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
    Use the completed item 19 reductions as the next measurement baseline.
    Acceptance: original timing-service contracts and game-step cadence pass on
    `a1200-fast`, matched gameplay regressions pass, and the release hardware and
    installer requirements accurately include the validated configuration.

14. **WHDLoad and release.** Installer that decodes the user's archive and verifies
    SHA-256; KickEmu slave with `WHDLF_EmulLineA`, CPU compatibility and AGA checks;
    deferred 68040 timing-service and original game-step cadence validation,
    68040 MMU test; deterministic LHA.
    Acceptance: the original/native timing gates pass on every additional claimed
    CPU configuration, and the playbook §14 release checklist passes, including
    real hardware. Two current 68040 cadence repeats pass all 58 original five-tick
    intervals; this does not establish hardware performance.
    Version 0.90 (16.10.2026) is in progress;
    current installer and 040 evidence is in `docs/release.md`. The owner will
    test an A1200 68040/40 with 32 MB and MMU. The refreshed, independently audited
    LHA includes the conversion-bounds,
    buffer-synchronization and heap changes; two clean builds reproduce it.
    Hardware testing has not yet been performed. The retained LHA predates the
    freed-handle caller guards; refresh the candidate and its applicable software
    evidence before hardware qualification.
    The original null-channel write that corrupted the WHDLoad privilege vector
    now has a paired relocation/store check. Release 030/NOMMU gameplay and
    normal quit pass with PRELOAD on/off; ordinary F10 and fresh-boot save/load
    persistence also pass. Fresh 040/MMU save/load, resumed gameplay and normal
    exit pass the shared functional MMU workflow, with translation/caches active
    and eight evidence-corruption controls. The cache-aware debugger now also
    observes the strict audio return and save/load dialog IDs; original-paired cleanup passes (see `docs/release.md`).
    Installer-only archive rejection passes on the host; the real Installer 43
    rejection requester still needs an interactive dismissal check.

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
