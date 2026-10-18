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
    The callback/dispatch shortcuts, shared saved-register copy, guarded HUnlock entry, shared Kalms kernel, nine drawing quick entries and shared changed-row screen copies bring
    the current runs to 537/537 PAL fields (baseline 660), with 57 of 82 requested intervals late. Landing takes
    253.07–253.30 ms/frame against the 83.3 ms request. Both retain all baseline
    player records and first-room/death pixels, but still have an extra death-phase
    frame relative to the original. Cadence acceptance is not met.
    The retained VBL and Time Manager fixtures pass with 13 and 12 rejection controls.
    See `docs/performance.md` for calibrated captures and the guarded replacement;
    previous explicit-frequency baselines ran too fast. D16 authorizes further
    equivalent, byte-guarded original-rendering replacements with original-paired
    validation; the cadence gate remains unchanged.
    The earlier C++ row-end comparison during screen CopyBits is rejected: four- and
    sixteen-row bands reduce conversion work but comparison/planning consume
    nearly all the saving (293.19/292.79 ms landing pilots). Neither establishes
    full display correctness or a repeatable material gain. See the rejected-pilot
    evidence in `docs/performance.md`. The shared register copy passes 28,388 native
    cases and the original-paired integration route.
    The HUnlock entry passes 256 native/full flag/CCR cases, 96 invalid-handle
    rejections and 32 original-paired freed-handle cases. Its earlier A/B repeat
    failed; its frozen B/C pair passes. The retained Kalms fixture passes
    193 native byte/ABI cases plus all 82 full planar images.
    The 192 KiB block-cache pilot is rejected: comparison and maintenance consume
    most of its conversion saving; the prototype is removed.
    The shared native row-comparison path passes 1,666 copy/ABI cases, including
    a full 640×480 copy, and all 82 game-frame boundaries against the prior build.
    Pixels, planes, copper colours, colour tables and player state match even
    on 21 frames that skip unchanged publication. First-room output and curated
    state match the pinned Macintosh reference. Eight corrupted-evidence controls
    and the shared sanitizer suite pass. The strict original cadence failure is
    retained; fewer publications do not waive any game-frame checks.
    A bounded BankRLELR transparent-run replacement passes 448 paired native
    original-instruction cases. Before installation it still needs run-length and
    clipping boundary cases, actual Macintosh pairing, byte guards and matched
    game timing. The dispatcher split and framebuffer-alignment experiments are
    removed; neither is a qualified production gain. Evidence is in performance.md.
    Next target broader full-dispatch overhead and hot rendering/conversion work;
    the remaining gap cannot be closed by small handle shortcuts alone. Keep
    equivalent rendering replacements available under D16.
    The bounded landing sprite census finds only 32% pixel reuse with an
    unbounded encoded-row cache, before validation and maintenance costs; no
    sprite cache is installed. See the pinned census in `docs/performance.md`.
    A forward-RLE C++ pilot passes 2,048 captured original-instruction cases
    but costs 1.773 times the original in repeated standalone batches. It is
    rejected without game integration; the original assembly remains installed.
    The isolated GetPixBaseAddr quick-entry pilot is also removed: its single
    landing result is 272.86 ms, still failing cadence and only about 3 ms below
    the retained baseline. Broader changes remain necessary.
    The nine drawing entries pass 1,152 fast/full comparisons, 126 guard
    rejections, four terminal-error controls, 192 actual exception-return ABI
    cases and all 82 full planar/viewports checks. The two repeated profiles
    match all baseline player records and endpoints. CopyBits and ClipRect
    retain full dispatch; the earlier ten-entry pilot remains unqualified.
    A synchronous Kalms CPU/blitter column prototype passes the 193-case native
    pixel/ABI fixture but regresses landing to 296.00 ms. A two-buffer overlap
    pilot using the shared blitter queue still costs 289.67 ms; its revised blit
    mapping also passes the 193-case oracle. Both are removed; neither establishes
    complete in-game planar correctness or improves the production baseline.
    A batched constant-mask CPU/blitter layout also passes the 193-case native
    oracle but regresses landing to 276.71 ms: its CPU stage alone costs 49.09 ms.
    It is removed; no hybrid converter is installed.
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
