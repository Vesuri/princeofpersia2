# Input and port-owned UI

`make ui` enables the VBI keyboard bridge and port-owned UI. The UI gate verifies
144 startup calls matching the Macintosh reference through menu initialization
and window setup, then stops at an explicit test-only checkpoint before
`Palette2CTab` (CODE 17+$0820, trap $AAA0). That service is implemented in the
ordinary build. The UI configuration omits the [native audio driver](audio.md).
The separate [integration gate](native-integration.md) executes startup through
the first playable room; the UI service fixtures alone do not prove gameplay.

`make ui-check` repeats original input and UI fixtures, checks native execution
and AGA output, and exercises rejection controls. Reports are in ignored
`tmp/ui-gate-*/report.json`. `--references UI UI_REPEAT INPUT INPUT_REPEAT` on
`tools/check_ui.py` reuses completed original captures. The gate leaves the
startup-checkpoint configuration built. Original archives, reference frames,
measured records and patched CODE bytes are never tracked.

## Input contract

The CIA keyboard interrupt retains raw press/release edges. VBI translates those
edges into the Macintosh KeyMap and EventRecords; polling cannot lose a short
press or manufacture another key-down. The Amiga physical-key mapping and CIA
ownership/handshake adapt the pinned Alone in the Dark implementation, with this
game's character/event behavior checked against D2.

The port-1 joystick is sampled at the same VBI and holds Amiga raw keys
(owner mapping): the stick holds the cursor keys and the second button
(POTINP bit 14) holds Up. Fire holds one key chosen from original state at
the press: Space while the mode word marks a cinematic, Ctrl
when the kid's room has a living character as the original closest-opponent
search counts them, otherwise Shift. DERIVED: the original ReadKeyboard
prefers Shift, so holding both would never reach Ctrl. Keyboard and joystick holds
share one Macintosh key position, so releasing either keeps a key the other
still holds. `tools/test_amiga_joystick.cpp` covers the decoding.
`tools/test_whdload.py --route-input` holds one input over 48 first-room updates
of a release WHDLoad run; the game ignores input over the first eight. These
runs use the pinned `fs-uae-arm` build, whose `monitor input` drives the
joystick. `tools/check_joystick.py` requires each joystick input to leave the
same kid record as its keyboard equivalent (fire is Ctrl in the first room,
which has a living guard), with controls that distinguish Right from no input,
Up and Shift-Right, and Ctrl-Right from Shift-Right. The Shift and cinematic selections are covered
by the host test only.

MEASURED: original ADB input for A, four arrows, Space, Return, Escape and Command-S
produces nine key-down events. KeyMap uses the Macintosh physical positions;
messages carry the ADB source bits, virtual key and character. The default event
mask is $FFEF, so releases update levels without key-up events. Command-S carries
command and mouse-up modifier bits. Two independent original runs agree on all
messages, modifiers and observed maps. Native VBI probes match that contract;
direct and cached `GetKeys` calls also overwrite seeded output buffers correctly.

The retained event queue supports mask selection, non-consuming `EventAvail`,
removal and `FlushEvents` stop masks. Synthetic sanitizer tests cover short edges,
masking, peeking, modifier levels, flush ordering and overflow. Queue overflow and
unmapped physical keys are named stops. The native rejection gate deliberately
overflows the raw queue. Shutdown verifies the previous CIA vector owner, in
addition to the existing View, VBI, exception-vector, heap and DMA/IRQ checks.

`WaitNextEvent` currently accepts the reached nonblocking form: zero sleep and
nil mouse region. Other forms stop explicitly. Mouse coordinates remain zero and
mouse-up is set because the owned interface is keyboard-operated. Held gameplay
keys are levels; autorepeat, international character composition and complete
Event Manager scheduling are not claimed by these bounded probes. VBI supplies a
monotonic event clock using six ticks per five PAL fields; original game pacing,
Time Manager and VBL task execution still require the timing gate.

## Reached UI paths and replacements

The following coverage combines the original opening survey, the 144-call native
startup prefix, and explicit CPU-executed service/original-routine fixtures.
Fixtures supply caller state after the pinned opening checkpoint; they are not
presented as naturally reached gameplay routes.

| Path | Port behavior | Positive probe |
|---|---|---|
| InitFonts, InitWindows, InitMenus, TEInit, InitDialogs, MaxApplZone | Owned initialization and invisible window-manager port; fixed reserved application zone; no Mac font, TextEdit or dialog drawing | Matching original/native startup prefix |
| GetKeys, GetNextEvent, EventAvail, WaitNextEvent, FlushEvents | Keyboard-backed levels and retained events in the bounded forms above | Original observer/ADB reference; native VBI API and direct/cached GetKeys probes; sanitizer mask/peek/flush checks |
| GetNewMBar, SetMenuBar, GetMenuHandle, AddResMenu, DrawMenuBar | Owned copies of the four original menu records; original command IDs and enable state; no bar or desk accessories | Startup prefix; menu fixture |
| New/End Game, Open/Save, Quit | Amiga-N/E/O/S/Q returns the original command to its original handler | Original/native MenuKey results; disabled/enabled/disabled Save control |
| Pause, Sound, Music, Hall of Fame, Restart | Amiga-P/T/M/H/R selects original action; P is an owned binding for the original unbound Pause item | Original/native command fixture, including positive original unbound-P control |
| About | Amiga-I selects the original About action; owned text screen replaces DrawAbout 600 | Original About returns; native VBI dismissal and full AGA pixel comparison; original unbound-I control |
| MainAlert 401/404 | Owned message with Return/Escape for OK | Original/native item 1 results and pixel probes |
| MainAlert 405 | Owned OK/Cancel prompt, Return/Escape | Both original/native outcomes 1 and 2; pixel probes |
| MainAlert 406 | Owned Yes/No prompt, Y/Return and N/Escape | Both original/native outcomes 1 and 2; pixel probes |
| CopyProtectionDialog | Byte-verified D6 branch to the original accepted continuation; acceptance flag set; original menu updates and epilogue retained | Original already-accepted fixture versus native initially-unaccepted fixture |
| Startup/game compression offers | D9 returns false without showing a prompt; compression command cannot be enabled or checked | Both original offers declined and both native seams returning zero; attempted menu re-enable control |
| Full Screen, HideMenuBar, ShowMenuBar | D3/D8 fixed client; no Mac menu hide/show drawing or display toggle | Original F shortcut positive control; native F unavailable; both guarded menu seams execute |
| Init/Hide/Show/ObscureCursor | No pointer pixels | Startup survey, sprite DMA disabled and complete owned output comparison |
| SelectWindow, FrontWindow, Show/HideWindow | Single owned client, no window chrome | Visible/hidden/visible owner checks; all eight planes remain a plain client background |

Menu labels and marks are metadata for the original handlers. Existing portable
menu-record helpers handle item text and mark changes. Unknown item owners,
unsupported callbacks, unknown dialog IDs and unsupported service forms stop;
they do not return fabricated success. Trap-address queries return stable authored
bridges for the measured queries, including cached `GetKeys`. Availability of an
address does not make an unimplemented service succeed.

The owned prompt uses the Amiga system's fixed font, measured as 10×9 in the pinned
native environment. It publishes a complete 512×384 monochrome client and restores
the prior client afterward. Seven prompt captures compare every logical pixel
with all eight actual AGA bitplanes, copper colours, mode and pointer setup.
Two native runs produce identical pixels. Text is confined to the owned content
area; neither Macintosh chrome nor sprite DMA is present. No original font or
pixel data is compiled into the replacement.

## Guarded seams and rejection evidence

`manifest/ui-inputs.sha256` pins the seven-patch input produced from the verified
application. `tools/ui_inputs.py` contains authored replacement instructions and
original segment/offset coordinates; original byte guards remain ignored.
Runtime verifies the complete input hash, compares original bytes before each
patch, and includes the authorized changes in its code-write audit. The seams
are CODE 2+$1A40/$3AE8/$3B76 and CODE 17+$245E/$4560/$463A/$6D42.

`manifest/ui-reference.sha256` pins the normalized original command and input
contract (messages, modifiers and observed maps, excluding volatile tick
values). Explicit completion markers independently require each original dialog
outcome. The native gate rejects a corrupt patch input, a changed original byte,
an unknown About ID and raw queue overflow. The same checker that accepts a
complete run rejects missing completion, incorrect dialog results, lost CIA or
heap restoration, a false Shell success, corrupt KeyMap/pixels/copper and enabled
sprite DMA. No timeout or absent probe is accepted as a result.

## Session file chooser

D8 replaces the reached Standard File selectors 1 and 2 with the existing Amiga
overlay and keyboard input queue. Save edits a name of up to 31 printable ASCII
characters; Open lists saved games with Up/Down selection. Return confirms and
Escape cancels. Existing save names require a second confirmation. Installed
assets cannot be overwritten. Unknown selectors, hooks and filters remain named
stops. The original game still creates, writes and reads its save document.

Both reference ports were inspected first; neither implements these Standard
File forms. The chooser reuses shared `MacFiles` lookup, ordering and metadata
and the existing port overlay. The application directory now has a `PROGDIR:`
backing path, allowing the original's application-folder saves; discovery covers
that directory as well as `Data`. The paired session gate validates a fresh save
and subsequent Open, including actual saved bytes and resumed gameplay. See
[gameplay coverage](gameplay-coverage.md) for completion and limits.
