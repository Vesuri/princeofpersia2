# Native Halestorm audio

`make audio` adds the D7 driver to the UI build. Application resource lookup
supplies port-owned `Jnth` 11, preceding the original `MDRV` fallback. The four-byte
owned entry uses the original C calling convention and remains callable after
`DetachResource`. The original application/resource fork is unchanged. Attempting
to execute a changed entry or fetch the original `MDRV` is a named stop.

The ordinary audio build includes the complete native startup services. The
separate [integration gate](native-integration.md) reaches the first playable room.
Audio acceptance here remains CPU-executed service replay on the fixed PAL 68020,
paired with original Macintosh title, gameplay, natural death and uninterrupted
opening-cutscene events. Its startup regression deliberately stops at a test-only
144-call checkpoint before `Palette2CTab`; that service is implemented in the
ordinary build. These audio fixtures do not establish timing or performance.
The timing gate passes on the current `a1200-030` target; see
[timing](timing.md) for its scope.

## Inputs and reuse

The application continues to load Mohawk songs from `MIDISnd.dat`/`NISMIDI.dat`
and raw effects from `DigiSnd.dat`/`NISDIGI.dat`, through the existing file services.
Its own sequencer submits individual notes to the driver; there is no replacement
MIDI parser or guessed song progression. The paired replay verifies each effect
payload against its source archive and retains its original ID, size and rate.
The application supplies 34 `INST` records and 34 format-1 `snd ` instruments.

`manifest/runtime-data.sha256` pins the application and all four archives.
`manifest/audio-replays.sha256` pins the two private acceptance replays.
`manifest/audio-inputs.sha256` pins the expanded MIDI Synth 3.45 image and its
512-byte quantized pitch table. `make audio-inputs` verifies those identities and
writes the table to ignored `build/audio/audio-pitch.bin`; generated headers contain
hashes only. The runtime checks that file before audio initialization.

Alone in the Dark's pinned native driver supplies logical/physical voice separation,
integer period calculations, unsigned-to-signed PCM conversion, phase-preserving
loop preparation and Paula register sequencing. Donor files/commit/hashes are in
`manifest/audio-sources.tsv` and `manifest/references.tsv`. `PaulaSample.h` is reused
unchanged; `SoundEffect.h` only gains this port's platform include guard. Prince's
format-1 instrument parser, pitch lookup and original logical allocation replace
the donor's game-specific format-2 and voice-count assumptions.

## Measured contract

MEASURED in the hash-guarded expanded driver (offsets below are relative to that
image, not Amiga addresses):

| Contract | Evidence |
|---|---|
| Entry/return | +$0000 / +$00EC; selector and argument are two C longwords; D2–D7/A0–A6 preserved |
| Initialization | Selector 21 twice, configuration 5 music / 5 normalized / 1 effect; selector 24 quality $000B |
| Direct synthesis | Selector 35 sets flags $7405; application sequencer uses note-on 9 and note-off 12 |
| Admission | +$34B6–$3570: matching released voice, free voice, released fallback; all five held means drop the new note |
| Note-off | +$333A: matching note/channel/owner gets duration zero and release count five |
| Release processing | +$219E: first synthesis block is skipped for a new note; release expires after seven following blocks, or earlier sample end |
| Source cursor | 185 output samples per low-quality block; the note-off path stops wrapping the source loop and plays its tail |
| Clock | State+$1A96 is the driver's clock; state+$13A2 counts work per active voice and block and is not elapsed time |
| Pitch | Table at +$7256, instrument base pitch and sample base note; original near-integer truncation retained |
| Effects | Selector 17 takes a 38-byte raw PCM descriptor; one logical effect slot replaces its predecessor; reached rate $2B110000 and no loops |
| Completion | +$32F6 calls once with descriptor context; selector 18 uses the stopping descriptor's callback, including null suppression |

The `Mohawk.ini` text advertises four MIDI voices; the measured runtime actually
requests five. Neither value is substituted for the other. `manifest/audio-reference.sha256` pins the consumed allocation/release contract.
The 27-call original fixture positively tests full-voice drops, released-voice reuse, release lifetime
and cursor advancement. Continuous paired logs also check release cursors.

## Paula rendering and ownership

There are four physical Paula channels and no software mixer. Logical admission
remains the original five-music/one-effect policy. The D7 donor's physical policy
uses a free channel, then steals the oldest audible music voice; music never steals
the effect channel. A stolen logical voice continues its original bookkeeping.
This authorized hardware reduction does not promise the original mixed waveform.

Each channel owns a bounded Chip RAM buffer. Signed sample conversion happens in
user mode. Integer periods use the PAL clock; high pitches use power-of-two sample
strides to keep the period at least 124. Odd loops retain their phase across word
alignment. One-shots reload two zero bytes. Note-off publishes the original source
tail from the measured logical cursor. Volume is bounded to Paula's six bits.

DMA is disabled and quiesced before replacing a channel's data; the attack is
latched before its reload is published. Audio interrupts and ADK modulation are
not used. Active system audio DMA is rejected before takeover. Shutdown stops all
four channels and restores ADK state alongside the existing DMA/IRQ/View/vector
restoration checks.

VBI only advances a small audio clock (six ticks per five PAL fields). Allocation,
conversion, completion processing and original callbacks run at user-mode service
points. Callbacks retain registers and condition codes; destinations must be owned
CODE or an exact application jump-table entry, with bounded context/stack storage.
Unmeasured selectors, effect loops, configuration/rate/extent violations and callback
queue overflow stop explicitly.

## Reproducing the gate

`make audio-check` generates original references, builds the portable sanitizer
fixtures, runs paired native cutscene and route replays, repeats the native route,
and exercises rejection controls. It leaves the ordinary audio build installed.
To reuse completed captures after `make audio-inputs`:

```sh
. amiga/env.sh
python3 tools/check_audio.py --references \
  ref/mame/runs/audio-route-6 ref/mame/runs/audio-cutscene-5 \
  ref/mame/runs/audio-fixture-2
```

Original audio calls/returns are stack-paired, including interrupted/nested calls.
The full original driver is byte-checked before internal probes are armed. Positive
route, natural-death and cutscene completion are mandatory. Private replay input is
hash-bound to its executable and contains no expected results for the runtime to
copy. The CPU executes GetResource, DetachResource and every driver call through
the owned resource, with deliberate register-clobbering callbacks.

The checker compares all D0 results, note admissions/pitches, callback count,
register preservation, original release cursors and independently calculated DMA
periods, volumes, complete converted-sample hashes and loop/tail extents. It also
requires actual enabled audio DMA in Chip RAM, disabled modulation, restored ADK
and the existing machine/heap restoration evidence. Replay uses original driver
clock values; it tests service semantics and real DMA publication, not matched
wall-clock gameplay pacing.

Rejection controls alter input hashes, the thunk, selector, callback owner and
original probe guard. Evidence controls remove completion, truncate events and
corrupt results, sample hashes, periods, Chip RAM status or restoration. Captures,
PCM, replay packets, traces, binaries and emulator disks remain ignored; only
source, facts, names and hashes are committed.

## Verified results

MEASURED: the title/gameplay/death route pairs 9,329 calls, 1,055 note decisions,
1,135 DMA publications (62 release tails) and 16 callbacks. Its two native event
and DMA logs are byte-identical. The uninterrupted opening cutscene pairs 7,549
calls, 782 note decisions, 858 DMA publications (62 release tails) and 14 callbacks.
Independent original route and cutscene captures regenerate the same pinned inputs.
The route contains 63 title, 69 gameplay and 40 post-death note decisions, with
883 earlier opening/transition notes.

The gate passes ten evidence-corruption controls, six native runtime controls and
an original full-image probe-guard rejection. The portable regression suite and
synthetic sanitizer tests for period arithmetic, odd-loop phase, stride and buffer
bounds pass. The ordinary audio build retains the 144-call startup boundary and
restores the OS. The detailed receipt is in ignored `tmp/audio-gate-*/report.json`.

## Session restart

MEASURED: the ordinary Command-R route stops music and effects, then calls driver
selectors 10 and 11. In the original MIDI Synth 3.45 these remove and restart the
output timer (+$090A and +$0914). The driver clock remains unchanged between the
paired calls. The native implementation preserves the stopped logical voices and
discards elapsed wall-clock ticks while suspended. The matched restart returns
to a completed playable frame with the same RNG, selected game state and pixels.
See [gameplay coverage](gameplay-coverage.md).

Vette's Bogas wrappers and Alone in the Dark's Halestorm dispatch were inspected
first; neither supplies this game's selector-10/11 contract. Existing shared Paula
helpers remain in use. Suspending active voices remains a named stop until its
original behaviour and Paula rendering have been measured.
