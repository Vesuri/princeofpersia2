# Original startup runtime

`make core` builds the bounded native startup implementation. `make core-check`
runs a fresh Macintosh reference, the portable input checks, native execution
comparisons and rejection controls. The existing `make amiga` and
`make scaffold-check` retain the empty boot/quit configuration. The core build is
an unfinished game executable: it returns DOS FAIL at the named unsupported
`InitGraf` service, after restoring the Amiga. It is not a release build.
The separate [display build](display.md) implements InitGraf and reaches the next
named stop, InitFonts; these bounded core checks remain unchanged.

## Input and execution boundary

The native loader reads the application's resource fork and an independently
hashed relocation plan before takeover. `manifest/runtime-data.sha256` pins the
application; `manifest/core-inputs.sha256` pins the generated plan and startup
trap prefix. `make core-inputs` derives the plan from verified original bytes and
the pinned decoder. The plan and its original instruction guards remain under
ignored `build/core/`; only hash constants enter the executable. Native and host
checks reject bad hashes, malformed resource extents, A5 layouts and jump tables.

Owner-authorized relocations redirect absolute low-memory accesses to A5-relative
shadows without changing instruction lengths or startup decisions. The 56 mapped
sites use checked original instruction bytes; a changed instruction is a loud
stop. The shadow starts at A5+$2000, above the 8,176-byte original A5 region.
The input application is immutable. No extracted code or relocation byte table is
tracked or embedded in the executable.

Original CODE 1 starts at resource offset $0010. It expands DATA/ZERO into the
22,266-byte below-A5 region and applies DREL itself. The loader preserves unloaded
near jump-table entries, initially resolving CODE 1 only. Original instructions
install the LoadSeg/ExitToShell hooks and generate their callable stubs; the native
LoadSeg service then resolves each requested segment. Original CODE 19 runs the
static initializers, loading CODE 21 and 22, before CODE 2 calls `InitGraf`.

The application heap is 3 MiB (3,145,728 bytes). Input storage, A5 world, stack and
service stacks are separate port allocations. This is not a final Fast RAM
requirement or a performance measurement. File and resource services also support the separately tested reference file
workloads. [File services](file-services.md) describes that integration, paired
service replays and save/restart evidence.

The reached memory/resource and trap-table services use the existing heap and
resource-map helpers. A040 is `ResrvMem`; A11E is `NewPtr`. Unknown services,
trap-table entries and invalid handles stop explicitly. Fatal
68020 exceptions receive named stops. The exception bridge runs services in user mode on a private stack, uses a separate
fault stack, restores the original exception vectors, and returns through
the normal Amiga handback path. A native code-image audit rejects writes outside
the two measured CODE data slots and authorized relocations.

## Paired evidence

MEASURED: native startup reaches CODE 2+$3E90, `InitGraf`, after the same 30
application CODE traps as the System 7.5.5 / IIci reference. The comparison excludes
jump-table and callable service-wrapper traps from both application traces.

The checker compares every below-A5 byte immediately after DATA/ZERO/DREL and
again after the original static initializers. Resource-handle identities, A5
pointers and code pointers are resolved in their respective executions. It checks
both complete near jump tables (excluding the Mac launcher’s unused first 32 reserved bytes), all loaded CODE bytes and both original-generated
trap stubs. Constructor changes are limited to the observed exit-registration
fields and count. An unreached stop, reordered trace or changed snapshot fails.

A port-owned native trap fixture separately exercises the seven observed Gestalt
selectors and SysEnvirons. Its answers match the original reference returns,
including the absent A/UX selector. This fixture is separate from the original
startup trace; the game has not reached those queries at this early stop.
The low-memory CPU flag is $03, distinct from the SysEnvirons processor value $04.

Runs use the pinned A1200/68020 PAL AGA configuration, 2 MB Chip and 8 MB Fast,
without JIT. Success requires the exact stop, heap integrity, restored exception
vectors, VERTB, View, DMA and interrupt masks, and a subsequent Shell FAIL marker.
A deliberate unsupported-service exit is not reported as game completion.

The gate includes altered application/plan hashes, an altered original instruction,
an unknown trap, an illegal instruction, an unexpected CODE write and a never-completing debugger observer.
It also rejects missing or corrupted evidence and exercises the empty scaffold
again. Logs and reports stay in ignored `tmp/core-gate-*`, `ref/` and
`amiga/.run-*`. To inspect a core run manually, set `CORE=1` and `CORE_REFERENCE`
to a completed reference directory when invoking `amiga/run.sh`.

Display, input, audio, continued gameplay, WHDLoad execution and release validation
remain at their existing gates. The reference is authoritative only for the
bounded startup path demonstrated here.
