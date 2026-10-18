# Open work

Version 0.90 release work (14) takes priority. Item 15 is closed by owner
acceptance under D17; the supported minimum remains 68030.
Item 14’s 68040 cadence, real-hardware and installer validation remain required. Gameplay coverage (12)
remains accepted under D10–D12 on `a1200-030`.
Under D15, cinematic deadline item 19.10 and diagnostic wrapper item 21 are complete.
Items remain open until their acceptance gates pass.

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
    record matches the recorded level-three entry and all level-four captures.
    Original counterfactuals establish gameplay effects on actor position,
    animation and persistent skeleton timers; these are not rendering scratch.
    Keep the earlier invalid opponent-record copies in the level-six/eight
    discovery routes as separate failed controls; the accepted alternate routes
    avoid those copies. Evidence and exact run/manifest identities are in
    `docs/gameplay-coverage.md`. Item 12's owner-authorized acceptance is not
    an exact-match result: preserve failed strict reports and named guards.
    The first-room A5−0B77 discrepancy is resolved as volume-date residue in
    a reused File Manager parameter block; original call/return provenance and
    narrowly scoped normalization are documented in `docs/native-integration.md`.
    New causal fixtures tie the first level-three torch mismatch and the later
    slot-0 position/animation decision to the shared RNG stream. A level-four
    seed-only fixture fails and remains a control. Level-six observers identify
    the music-completion gate and separate earlier note enqueue timestamps from
    shorter delivery delays on the native driver.
    These findings do not close the other routes or waive strict comparisons.
    The 0x2200-byte context snapshot omits most persistent per-room opponent
    records. Aligning five persistent skeleton timers as well as seed/context
    reproduces the level-four actor, RNG and frame endpoint on the original.
    The common 2,952-draw RNG sequence, shifted ambient caller and randomized
    skeleton countdowns explain the first persistent timer mismatch.
    Flame observations show the
    shadow sequence running one step ahead for 80 consecutive paired states;
    the paired death-sound gate and shadow-creation observers establish that
    sound completion starts this lead. Driver-clock queries also
    expose different batching during the level-six music gate; causal clock
    replay passes its unchanged-value control but fails to reproduce the native
    schedule. Paired queued timestamps and delivery observations now establish
    that both inputs differ; the invalid pre-intervention and exhausted-response
    controls remain failures. Finish the per-route closure audit of the other
    early exits and terminal RNG
    differences before removing this item.
    Acceptance (D18): verify the causes and gameplay effects against the
    original, accept established timing-dependent differences, and fix service
    defects. Identical frame-by-frame outcomes are not required. Check the other
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
