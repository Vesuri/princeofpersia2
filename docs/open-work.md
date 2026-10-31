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
    `docs/release.md` and `manifest/release-091.sha256`. Version 0.92
    (31.10.2026) adds joystick input and the requester stop reason; by owner
    decision it reruns only the checks for those changes
    (`manifest/release-092.sha256`).
    The reported Zorro III graphics failure is addressed by preserving native
    32-bit pointers in StripAddress; see the new regression evidence in
    `docs/release.md`.
    Remaining acceptance: the owner tests installation, gameplay/audio,
    save/load, Amiga-Q/F10, PRELOAD on/off and MMU on the physical A1200
    68040/40 with 32 MB. Record the result and resolve any failure before
    closing this item. The requested a1200-030 emulator play session does not
    replace this gate. The audited 0.92 archive is attached to the GitHub release.

15. **Input — stops on unusual keys and long input bursts.**
    The owner once saw the "could not start" requester after F10 while a joystick
    direction may have been stuck. It did not reproduce, and that requester now
    shows the stop reason. Two named stops could explain it (INFERRED). First,
    raw keys with no Macintosh position stop play with INPUT / UNMAPPED RAW KEY:
    $0E $1C $2B $2C $30 $3B $3C $43 $47 $48 $49 $4A $4B $5A $5B $5C $5D $5E.
    That list includes the international <> key $30. Second, 64 unread key-down
    events are a hard stop, where Mac OS drops the oldest event instead. Decide
    with the owner whether these keys are ignored and whether the queue drops
    events, as the original OS would. If the report recurs, record its requester
    text.
