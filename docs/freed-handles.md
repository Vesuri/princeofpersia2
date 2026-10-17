# Freed-handle boundary

Item 18 is investigated against System 7.5.5 on the pinned Macintosh IIci.
A freed master is not a valid handle: its contents link other free slots.
Its error cannot be inferred from slot membership alone.

## Original layout controls

MEASURED: two fresh originals, `freed-layout-final-1` and
`freed-layout-final-2`, produce identical 700 result records. The opt-in
`tools/mac_freed_handles.lua` fixture uses the existing private-zone/trap
coroutine protocol from `mac_heap.lua` (adapted from Alone in the Dark).
It allocates a 64 KiB private zone with 64 initial master slots, allocates
1, 2, 3, 8, 32, 64 or 65 handles, disposes them in each order, and calls
HUnlock and HPurge on every disposed alias. Twenty-eight live-handle calls
provide successful controls. The authored trap trampoline uses the existing diagnostic stack area; original
game instructions and input files stay unchanged.

| Original result | Calls | Whole private zone unchanged |
|---|---:|---|
| `memWZErr` (−111) | 682 | All |
| `memAZErr` (−113) | 8 | All |
| `nilHandleErr` (−109) | 4 | All |
| Success | 6 | Three HUnlock calls only |

The four nil errors have zero free links. The eight `memAZErr` calls have a
free link to zone+$40, adjacent to the first master-block boundary. The six
successful calls have a link to zone+$3C, at that block's first slot; the
word eight bytes before that target is `$40000108`. Each successful HPurge
changes one private-zone byte. The fixture records each mutation and explicitly
restores the private-zone snapshot before continuing; it never labels these
three operations as non-mutating errors. It restores the original zone and
successfully disposes its private arena before reporting completion.

DERIVED: allocation/disposal layout changes the apparent pointee and therefore
the observed result. These tests establish a boundary-dependent classification,
not a portable algorithm for arbitrary invalid pointers. Native heap block
headers and free-slot order differ from Macintosh layouts; applying the Mac
boundary offsets to the native heap would invent behavior. The original's
success and mutation cases are controls against broadening support, not new
supported operations.

Fresh original `freed-game-2` also measures UnloadLevel's HPurge at CODE
3+$63AA: D0/MemError are −111 and the non-null master stays unchanged. The
assisted expired-story route returns normally and its curated checkpoints
match the existing reference. The fresh `freed-game-3` and `freed-ending-1` additionally cover all three
follow-up palette HPurge calls, each preserving its slot with −111. Three
altered live-call controls reject changed
errors, call sites and missing completion. `freed-game-1` is a rejected harness
attempt: its required cinematic observer was missing. Native `.run-hCJT38`
is also retained as a rejected control: limiting HPurge to the demo call alone
correctly stopped at this additional caller, with heap and hardware restored.

The earlier isolated application-zone fixture returns `memAZErr`, while the
scene-20, ending and demo cleanup aliases return `memWZErr`. Their separate
pinned original contracts remain authoritative for those game calls.
The first exploratory run, `freed-layout-pilot-2`, deliberately fails its
whole-zone preservation assertion at the first successful HPurge mutation;
that failed capture is retained. `freed-layout-pilot-1` failed HFS preparation
before the emulator ran. Neither is an accepted run.

## Native support

The port supports a freed-slot error only at these original call sites:

| Service | Original CODE location | Original measured result |
|---|---|---|
| HPurge | 3+$5F9A (demo), +$63AA (UnloadLevel) | −111 in demo/story; −113 at ending UnloadLevel |
| HUnlock | 15+$29AA, +$29C2, +$29DA, palette cleanup | D0 and MemError = −111 |
| HPurge | 15+$29B0, +$29C8, +$29E0, following the same aliases | D0 and MemError = −111 |

`FreedHandleCall.h` centralizes these game-specific locations. The dispatcher
also requires `MacHeap::isFreeHandleSlot`; it does not dereference an unknown
pointer to classify it. The error preserves the free slot and reports negative
condition codes. At UnloadLevel +$63AA, the native error remains −111 even when
the original's incidental heap layout returns −113. This is not an exact error
value match. MEASURED in `freed-ending-memory-2`: the original writes −113, then
clears MemError before any CPU read. The physical alias is checked against the
complete first KiB of virtual low memory; positive CPU reads and writes validate
the observer. DERIVED from the byte-guarded original next instruction: D0 is
immediately discarded by MOVEQ. Thus the retained negative error does not change
the observed game decisions. It is not permission to normalize errors the game
uses. Four altered-observation controls reject an inserted read, missing alias
check, missing positive reads and a missing −113 write.
The first memory observer (`freed-ending-memory-1`) watched a virtual address
as physical memory and saw no accesses; it is retained as rejected evidence. Reallocated live slots use normal service handling. Other
freed-slot callers, arbitrary pointers, misaligned addresses and unsupported
operations retain named stops. In particular, HPurge no longer treats every
free slot at every caller as the demo's measured error.

Vette's allocation registry and permanent-handle checks and Alone in the Dark's
MacHeap/service adapters were inspected first. Alone in the Dark already uses
the shared free-slot validator for its measured GetHandleSize alias; it does
not supply a general HUnlock/HPurge error classifier. No common heap algorithm
was added or copied. The new predicate contains this game's call sites only;
shared heap validation and free-list operations are unchanged.

## Reproduction and checks

Source `amiga/env.sh`, then run each reference with a fresh name:

```sh
POP2_INTEGRATION=1 POP2_NULL_DISPOSE=1 POP2_FREED_PROBE=1 \
  python3 tools/run_reference.py --name freed-layout-final-1 --deadline 600
python3 tools/check_freed_handles.py ref/mame/runs/freed-layout-final-1 --controls
make host-tests
```

The reference runner verifies the original inputs before execution. Evidence
hashes are in `manifest/freed-handles-evidence.sha256`; original game alias
hashes remain in their existing manifests. Eight altered-evidence controls
reject missing completion/restoration, changed errors and D0, concealed heap
mutation, missing valid-handle controls and reordered cases.

The host policy/heap fixture tests all 14 allocation/disposal layouts,
33,600 caller/service combinations, unchanged whole-heap bytes, invalid
addresses and a reused live slot. A control with the previous unrestricted
HPurge predicate fails. These host checks establish policy and ownership;
they do not claim native execution of all Macintosh invalid-layout cases.

For the read/write observation, run a fresh assisted ending reference with
`POP2_DISPLAY_ROUTE=1 POP2_NATURAL_OPENING=1 POP2_GAMEPLAY=route
POP2_ROUTE_DISCOVERY=1 POP2_ROUTE_ASSIST=1 POP2_ROUTE_ENDING=1
POP2_ROUTE_RESOURCE_TRACE=1 POP2_FREED_LIVE=1 POP2_FREED_MEMERROR=1`.
In another terminal, use `tools/replay_gameplay_assisted.py` with the new run,
`--reference ref/mame/runs/gameplay-assisted-ending-10` and
`--manifest manifest/gameplay-assisted-ending.sha256`. The launcher requires
the read/write observation as well as normal ending completion. The story
variant uses `POP2_ROUTE_EXPIRED_PROBE=1 POP2_CUTSCENE_OBSERVER=1` instead of
`POP2_ROUTE_ENDING`; at its first checkpoint, the existing assistance tool sets
minutes to zero and the route-command input requests one neutral frame.
These are the already accepted assisted routes, not new gameplay shortcuts.

Native `amiga/.run-AtOdet` passes all four expired-story checkpoints, reaches
one freed HUnlock, completes normally and restores the heap and hardware.
Three altered native records (zero/missing hits and missing completion) are
rejected. Native `amiga/.run-CIlJ3d` passes all 81 assisted-ending checkpoints,
385 frames and 387 consumed keys, reaches three freed HUnlocks, and restores
the heap and hardware. Both compare against their pinned Macintosh routes with
zero deferred mismatches. The earlier native `.run-i3cO2P` stopped at the
previously unlisted palette HPurge; it remains a failed guard control.
The full host suite and final 33,600-case policy test pass. `make release`
restores the ordinary build and its no-runtime-hashing/tracing audit passes.
The retained item-14 LHA has not been repackaged or newly qualified by this work.
