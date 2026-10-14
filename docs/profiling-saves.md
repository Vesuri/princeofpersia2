# Profiling saves

The original Macintosh game's SaveGame routine creates the profiling files.
`profiling-saves-1` completed all 14 writes and volume flushes. The exported set
contains 14 distinct 100-byte data forks; `manifest/profiling-saves.sha256` pins
the data and sidecars without committing original bytes.
They are assisted **level-entry checkpoints**, not exact mid-room emulator
snapshots and not evidence that a level's route has been completed.

Private output belongs in ignored `tmp/profiling-saves/`. Each `Profile NN` data
fork travels with `Profile NN.rsrc` and `Profile NN.finfo`. Keep the three files
together. The resource fork is empty; Finder metadata identifies a game save.
`manifest.json` records the source run and hashes, preserving the original
MacBinary wrappers separately in that run's `profile-wrappers/` directory.

To include the saves in an ordinary release play session:

```sh
PROFILE_SAVE_DIRECTORY="$PWD/tmp/profiling-saves" make play AMIGA_SETUP=a1200-030
```

Use Amiga+O and select the desired `Profile NN`. Set `PROFILE_SAVE_LEVEL=NN`
to stage just one level. Hash verification happens on the host during staging;
ordinary game startup does not scan the files. Staging refuses to overwrite an
existing file. The files can also be copied, with their sidecars, beside the
Amiga executable.

Recreate the private set with an opted-in original run:

```sh
POP2_GAMEPLAY=route POP2_DISPLAY_ROUTE=1 POP2_ROUTE_DISCOVERY=1 \
POP2_ROUTE_ASSIST=1 POP2_ROUTE_DIRECT_KEYS=1 POP2_PROFILE_SAVES=1 \
python3 tools/run_reference.py --name profiling-saves-new --deadline 2400
```

While that run is active, use another terminal:

```sh
python3 tools/create_profile_saves.py ref/mame/runs/profiling-saves-new
```

After it exits successfully, export the flushed disk without modifying it:

```sh
python3 tools/export_profile_saves.py ref/mame/runs/profiling-saves-new \
  --destination tmp/profiling-saves-new
```

The fixture requests each original level-load path, invokes the byte-guarded
original SaveGame ABI on a private stack, requires success, flushes the original
volume, and restores the caller's registers. It does not manufacture save bytes.
The extractor uses the same MacBinary-to-AFI1 conversion as the verified Mac
save import gate. A separate native load check must establish live load evidence;
export and hash verification alone do not establish gameplay compatibility.

MEASURED: native `.run-l8KXUr` loads the unchanged `Profile 14` through the
ordinary Open dialog on `a1200-030`, completes the loaded first frame and a
subsequent gameplay frame in level 14, and restores hardware/heap/input correctly.
`check_profile_save.py` checks that result and the file hashes. All 14 exports
stage correctly and repeat-export hashes match; live load evidence for this set
currently covers level 14. The earlier Macintosh level-one import gate remains
separate. The ordinary release build passes its no-runtime-hashing/tracing audit.
