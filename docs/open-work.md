# Open work

14. **WHDLoad and release — hardware qualification.**
    The owner confirms that guard-death/Enter now restarts into playable gameplay
    on the updated a1200-030 release. The observer separately records the formerly
    fatal locked-handle result followed by a completed restart frame in level 2,
    room 2. Original-paired service regressions also pass; see `docs/release.md`.
    The software gates for version 0.91 (27.10.2026) pass: reproducible clean builds and LHA,
    independent archive audit, Installer 43 fresh/reuse/replace/repair and
    interactive damaged-archive rejection, release WHDLoad PRELOAD on/off,
    normal quit and F10, 030 and 040/MMU fresh-boot save/load, and the retained
    original-paired timing/cadence gates. Evidence and archive identity are in
    `docs/release.md` and `manifest/release-091.sha256`.
    The reported Zorro III graphics failure is addressed by preserving native
    32-bit pointers in StripAddress; see the new regression evidence in
    `docs/release.md`.
    Remaining acceptance: the owner tests installation, gameplay/audio,
    save/load, Amiga-Q/F10, PRELOAD on/off and MMU on the physical A1200
    68040/40 with 32 MB. Record the result and resolve any failure before
    closing this item. The requested a1200-030 emulator play session does not
    replace this gate. The audited 0.91 archive is attached to the GitHub release.
