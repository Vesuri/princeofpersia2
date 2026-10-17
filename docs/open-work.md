# Open work

Version 0.90 release work (14) takes priority. Under D14, 68020 support (15)
is no longer deferred beyond 0.90 and retains its acceptance gate.
Item 14’s 68040 cadence, real-hardware and installer validation remain required. Gameplay coverage (12)
remains accepted under D10–D12 on `a1200-030`.
Under D15, cinematic deadline item 19.10 and diagnostic wrapper item 21 are complete.
Items remain open until their acceptance gates pass.

15. **68020 support.** Measure and optimize
    the fixed 68020 configuration without changing original game decisions.
    The byte-guarded SaveRect inner-loop replacement passes 672 paired original
    Macintosh/native cases, full memory/register/CCR comparisons, rejection
    controls and two matched opening runs. Inactive copper palette reuse also
    passes original-table checks across 82 publications and a palette-only change probe.
    The zone-service callback shortcut brings the current runs to 614/613
    PAL fields (baseline 660), with 60/61 of 82 requested intervals late. Landing takes
    295.06–295.36 ms/frame against the 83.3 ms request. Both retain all baseline
    player records and first-room/death pixels, but still have an extra death-phase
    frame relative to the original. Cadence acceptance is not met.
    The retained VBL and Time Manager fixtures pass with 13 and 12 rejection controls.
    See `docs/performance.md` for calibrated captures and the guarded replacement;
    previous explicit-frequency baselines ran too fast. D16 authorizes further
    equivalent, byte-guarded original-rendering replacements with original-paired
    validation; the cadence gate remains unchanged.
    Row-end comparison during screen CopyBits is also rejected: four- and
    sixteen-row bands reduce conversion work but comparison/planning consume
    nearly all the saving (293.19/292.79 ms landing pilots). Neither establishes
    full display correctness or a repeatable material gain. Next investigate
    the remaining dispatch cost with matched measurements, preserving ownership,
    callback and pending-service guards; keep rendering replacements available
    under D16. See the rejected-pilot evidence in `docs/performance.md`.
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

22. **Revalidate accelerated timing with the corrected emulator clock.** The
    retained item-19 68030 profiler uses cycle unit 32 at requested 50 MHz,
    instead of the corrected integer unit 36. The legacy native emulator also
    predates AmigaXDev’s explicit-frequency fix. Preserve earlier captures as
    historical measurements; do not treat them as calibrated target timings.
    Re-run the claimed 68030 cadence and performance budgets with the shared
    clock-corrected build, and review affected 68040 release evidence while
    retaining the documented limits of its timing model. Keep the original
    workloads and acceptance budgets. Acceptance: verify live clock metadata,
    obtain repeated matched-workload measurements and positive timing gates,
    update the pins and claims, and retain failed controls. This is separate
    from item 15’s fixed-68020 qualification and item 14’s physical-hardware gate.
