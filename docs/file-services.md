# File services

The native startup still stops at `InitGraf`. File-service replays and resource
probes execute separately from startup; they establish the file/resource service
coverage below, not continued gameplay or complete File Manager / Resource Manager
coverage.

## Original observations

`POP2_FILES=1 python3 tools/run_reference.py` adds a read-only File Manager
observer. It records original CODE callers, input/output parameter blocks,
Macintosh errors, and transferred bytes under the ignored reference directory.
Nested System 7 implementation calls are excluded from payload capture. Missing
returns and asynchronous calls without completion observation fail explicitly.

`python3 tools/check_file_reference.py` makes two fresh runs and requires the
same normalized file operations. It checks every captured transfer against the
pinned installed forks, including offsets, EOF, returned positions and errors.
It also rejects missing completion, altered or truncated reads, changed write
sources and altered positions. Existing runs can be checked using
`--references RUN1 RUN2`.

MEASURED: the opening workload contains 93 paired file calls, including 16 reads
and one write. It reads and rewrites the 16-byte `Prince2.opt` data fork, reads
`Mohawk.ini`, and reads headers, tables and payloads from all four `MHWK` archives.
The original implements archive interpretation; file services supply its bytes.
The host transfer replay exercises 39 open/read/write/close/seek/EOF operations
from this workload through the portable `FileTransfers` implementation, using
private writable copies. It is not native acceptance evidence.

## Original persistence fixture

`POP2_FILES=1 POP2_FILE_ROUTINES=1 python3 tools/run_reference.py` first reaches
the normal opening checkpoint. It then calls the byte-guarded original
`InitHallOfFame`, `WriteHallOfFameFile`, `SaveGame` and `ResumeGame` routines with
explicit caller arguments and a separate slice of the application stack. The
original initialized level state is required; the harness supplies no game-state
values. This tests file routines directly, not the save dialog or gameplay route.

The original writes `Data:Prince.hof` and `Data:Fixture Save`. A harness-authored
`PBFlushVolSync` caller must complete successfully before MAME closes the image.
Exiting the emulator alone is insufficient evidence of persistence.

Set `POP2_RESTART_FROM=RUN` along with those flags for a fresh-system boot that
imports the flushed options, Hall of Fame and save forks from a completed source
run. Exports are preserved with their own SHA-256 manifest and verified before
import. The restarted original reads Hall of Fame and resumes the saved game.
The persisted-file startup is a distinct workload, pinned in
`manifest/reference-file-restart.sha256`; the original fresh-install baseline is
unchanged. `python3 tools/check_file_persistence.py WRITTEN RESTARTED` compares
original write buffers, exported forks and fresh-boot read buffers and requires
positive routine/flush completion. Two restart runs reproduce the pinned result.
The native replay supplies the same arguments and write buffers to the implemented
Macintosh traps. MEASURED: 39 save-phase calls and 22 restart-phase calls complete;
the options, Hall of Fame and save bytes match the original, including reads after
a fresh Amiga boot. Each transfer closes its DOS handle, and an explicit flush
call completes before the run exits. The restart imports all nine data/resource/AFI1
files through a checked SHA-256 manifest. `check_native_persistence.py` checks the
import, read buffers, final files and metadata; seven rejection controls cover
missing completion and damaged payloads, resource forks, manifests and metadata.

## Native integration

The generated, pinned `file-catalog.bin` contains original names, fork lengths and
SHA-256s. Native preparation verifies the catalog and every AFI1 metadata record.
Each original fork is verified before its first open or read. Subsequent transfers
use the existing catalog and file marks. All generated inputs and staged game
files remain ignored.

The Line-A bridge runs services in user mode on a private stack. Exec cache calls
use the original exception vectors. DOS operations temporarily restore the OS
vectors and VERTB server chain, permit scheduling, then restore the port's
ownership. No DOS call executes on the exception stack.

Native synchronous open, read, write, close, EOF, seek, create, flush, file metadata,
volume selection/information, catalog queries and `PBGetFCBInfo` dispatch are
connected to the catalog/backend. `FSMakeFSSpec` handles the measured existing and
missing-file cases. Unimplemented selectors, invalid memory and failed immutable
fork verification remain named stops. Positive catalog enumeration is not reached
by this reference and remains unsupported.

MEASURED: the complete 93-call opening file trace replays on the Amiga, followed
by an explicit flush. All 17 transfers match original bytes, errors, actual counts
and file positions. Catalog metadata, canonical names and FCB facts are checked;
volume capacity and directory dates describe the actual DOS backing. Native IDs
and the simplified root/application/Data hierarchy are checked independently of
HFS identifiers. The verified Macintosh installation clears Finder's initialized
flag; native metadata exposes the same installed state without changing original
input files.

New files are rediscovered from their AFI1 companions and paired forks at startup.
Writes update modification times and persist Finder metadata. `Prince2.opt` is
writable user state with a 16-byte data fork; its resource fork remains pinned.
Under D9, writes to installed game assets remain unsupported. Original read/write
open permissions alone do not imply a write: the audio archive reader requests
those permissions while reading.

The diagnostic `POP2_FILE_PROBE` build opens, reads and closes the original options
file through Macintosh traps. It deliberately waits inside a DOS window to prove
scheduling and VBI execution. `tools/check_native_file_probe.py` requires the
original options bytes, successful operations, positive scheduling evidence, and
the existing paired core startup/OS-restoration checks. It also opens `MIDISnd.dat`
and compares `PBGetFCBInfo` flags, logical length and file mark with the original
trace. Native file IDs, parent IDs and physical allocation are checked against the
native catalog, not equated to HFS disk addresses. Pass `RUN ORIGINAL_FILE_RUN` to
the verifier and set `CORE_REFERENCE` to a completed ordinary reference.

## Resource integration

`POP2_RESOURCES=1` adds a read-only resource observer to the original run. It
records paired arguments/results, resource names, loaded/unloaded handle state
and complete loaded payloads. `tools/report_resources.py RUN` verifies the
completed reference, follows resource-file lookup order and checks captured
payloads and `GetResInfo` metadata against the pinned resource forks.

MEASURED: the opening workload has 73 paired resource/FSSpec calls and 31 loaded
payload captures. Every captured payload matches its source, including repeated
lookups. The missing `Jnth` lookup returns nil; the observed low-memory error word
is zero. This is not evidence of an explicit `ResError` return for that missing
lookup. The native probe also checks all 11 observed FSSpec cases, including missing
files, canonical spelling, parent identity and Macintosh error returns.

The native directory supports the application and dynamically opened resource
forks, with a 64 KiB read cache per nonresident fork. Its total capacity is 4,096
resources, while each fork retains the parser's 768-resource limit. The measured
application/Prince/Kid/NIS/options chain contains 1,123 resources.
Open, current-file selection, lookup through older files, resource information,
loading, detachment and closing are connected to the runtime. Dirty resource
publication remains unsupported; D9 excludes in-place data packing.

The diagnostic `POP2_RESOURCE_PROBE` opens Prince, Kid, NIS and options resource
forks. The options filename is passed directly from its original CODE 2+$4284
address, as in the measured original call. It retrieves SHAP 25002 and DSLV 6400, obtains resource sizes and DSLV
metadata, detaches both handles, and closes the files. The complete 9,105-byte
SHAP and 176,668-byte DSLV bodies remain readable and match original observations.
`tools/check_native_resource_probe.py NATIVE ORIGINAL_RESOURCES ORIGINAL_CORE`
checks those bytes, metadata and the ordinary startup/OS-restoration proof.
`tools/check_resource_controls.py` takes the same arguments and rejects eight
missing or damaged evidence cases, after proving the untouched copies pass.
These bounded probes do not establish complete Resource Manager coverage.

## Reproduction and coverage

`make files-check` builds the host tests, obtains two original opening traces and
an original save/restart pair, then runs fresh native opening, save/restart,
resource/FSSpec and file-scheduling probes, plus changed-options and immutable-asset
write controls. Every native positive also passes the
core initialization and OS-restoration checker. Evidence and manifests stay under
ignored `tmp/`, `ref/` and `amiga/.run-*`; a complete gate writes `FILES GATE PASS`.
For development, `tools/check_files.py --references OPENING REPEAT SAVE RESTART`
reuses completed original observations and still runs fresh native probes.
`--core-reference RUN` selects a separate ordinary startup reference if needed.

`manifest/file-replays.sha256` pins normalized replay inputs. Generation preserves
documented arguments and original write bytes, binds runtime pointers and file
references, and follows the observed GetFInfo-to-SetFInfo date dependency. It does
not copy unused stack bytes or volatile original timestamps into the fixture.
The interpreter invokes Macintosh traps; the original implements Mohawk archive
interpretation. These service tests do not execute the full native opening scene,
which still requires item 8's display services.

`tools/check_resource_census.py` compares the runtime C++ reader with the independent
extraction helper for every installed resource. MEASURED: all 32 nonempty resource
forks and all 2,702 resources agree on type, ID, attributes, name, length and complete
payload SHA-256. Synthetic reader tests separately cover malformed extents,
capacity limits and failed reads. This host census complements the native paired
resource probes; it is not evidence that every resource was used during gameplay.

The changed-options control toggles one bit in the native read buffer and writes
all 16 bytes through the Macintosh file service, then imports those native-written
forks and metadata into a fresh boot. It checks the new bytes and every imported
file hash after the read. This tests persistence of changed user state, not the
semantics of a particular preference or its UI. A separate control attempts the
same write against `MIDISnd.dat`; it must reach `PBWrite`, stop with
`FILES / UNSUPPORTED OPERATION`, restore the OS, and leave all three archive
fork/metadata hashes unchanged. The successful options write is its positive
control. Run `tools/check_native_settings.py SAVE ORIGINAL_FILES ORIGINAL_CORE`
to reproduce these controls independently.
