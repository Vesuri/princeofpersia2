# Open work

14. **WHDLoad and release — physical-hardware qualification.** All software
    gates for version 0.90 (19.10.2026) pass: reproducible clean builds and LHA,
    independent archive audit, Installer 43 fresh/reuse/replace/repair and
    interactive damaged-archive rejection, release WHDLoad PRELOAD on/off,
    normal quit and F10, 030 and 040/MMU fresh-boot save/load, and the retained
    original-paired timing/cadence gates. Evidence and archive identity are in
    `docs/release.md` and `manifest/release14-current.sha256`.
    Remaining acceptance: the owner tests installation, gameplay/audio,
    save/load, Amiga-Q/F10, PRELOAD on/off and MMU on the physical A1200
    68040/40 with 32 MB. Record the result and resolve any failure before
    closing this item. The requested a1200-030 emulator play session does not
    replace this gate. The archive is built and audited, not published.
