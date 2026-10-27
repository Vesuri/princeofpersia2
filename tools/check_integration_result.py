"""Require original native title, first room, gameplay input and restoration."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from check_core_result import restored
from check_display_scenes import decode_planes, decode_copper
from display_frames import frames
from check_title_state import check_reference as title_state_reference
from extract_original_data import ROOT, pins


def require(ok, reason):
    if not ok:
        raise RuntimeError('INTEGRATION / ' + reason)


def crop(data):
    require(len(data) == 640 * 480, 'SCREEN EXTENT')
    return b''.join(data[y*640+65:y*640+577] for y in range(48, 432))


def normalized_reference_a5(raw):
    data = bytearray(raw)
    require(len(data) == 22266, 'REFERENCE A5 EXTENT')
    # Original cursor-idle TickCount and TMTask countdown.
    for distance in (0x53aa, 0x7aa):
        data[-distance:-distance+4] = bytes(4)
    return volume_date_normalized(data)


def volume_date_normalized(raw):
    data = bytearray(raw)
    require(len(data) == 22266, 'REFERENCE A5 EXTENT')
    # The shared file PB at A5-0xb9a retains the high word of ioVLsMod
    # (PB+34) after HGetVInfo. SetFPos clears its low word. This is host
    # volume metadata, not game state; check_reference_file_scratch.py
    # verifies the original call/return provenance and exact two-byte scope.
    data[-0xb78:-0xb76] = bytes(2)
    return bytes(data)


def pinned_reference(reference, name):
    # A '-a5.normalized.bin' pin hashes the raw A5 snapshot with only the
    # volume-date residue zeroed; every other byte remains exact.
    if name.endswith('-a5.normalized.bin'):
        return volume_date_normalized((reference/name.replace('.normalized.bin', '.bin')).read_bytes())
    return (reference/name).read_bytes()


def reference_state(reference):
    result = [frames(reference)]
    for name, digest in pins(ROOT/'manifest/integration-reference.sha256').items():
        if name == 'first-room-a5.normalized.bin':
            data = normalized_reference_a5((reference/'first-room-a5.bin').read_bytes())
        else:
            require(name == 'first-room-context.bin', 'REFERENCE INPUT NAME')
            data = (reference/name).read_bytes()
            require(len(data) == 0x2200, 'REFERENCE CONTEXT EXTENT')
        require(hashlib.sha256(data).hexdigest() == digest, 'REFERENCE STATE PIN '+name)
        result.append(data)
    return result


def check(run, reference, title_reference, *, right_expected=True, setup="a1200-fast", title_run=None):
    reference_state(reference)
    title = title_state_reference(title_reference, 16)
    log = (run/'gdb-out.log').read_text()
    restored(run, log, setup)
    require('CORE REASON INTEGRATION / ROUTE COMPLETE\n' in log, 'ROUTE COMPLETION')
    require('CORE EXCEPTION vector=0 pc=00000000\n' in log, 'EXCEPTION')
    require('INTEGRATION AUDIT code=1 input=1\n' in log, 'CODE AND INPUT AUDITS')
    require('INTEGRATION RESTORED\n' in log, 'RESTORATION MILESTONE')
    space = re.findall(r'^INTEGRATION SPACE events=(\d+)$', log, re.M)
    require(len(space) == 1 and int(space[0]) > 0, 'ORIGINAL GAME MUST CONSUME SPACE TO START')
    reads = re.findall(r'^INTEGRATION INPUT right_reads=(\d+)$', log, re.M)
    require(len(reads) == 1, 'INPUT READ COUNT')
    require(int(reads[0]) > 0 if right_expected else int(reads[0]) == 0,
            'ORIGINAL GAME MUST READ RIGHT' if right_expected else 'NO-RIGHT CONTROL MUST HAVE ZERO READS')
    stages = re.findall(r'^INTEGRATION STAGE (\d+) fields=\d+ right=\d+ kind=(\d+)$', log, re.M)
    require([int(s) for s, _ in stages] == [1, 2, 3, 4], 'ORDERED TITLE AND GAMEPLAY MILESTONES')
    require(stages[-1][1] == '3', 'ORIGINAL DEATH ROUTINE COMPLETION')
    title_source = title_run or run
    title_log = (title_source/'gdb-out.log').read_text()
    if title_run:
        check(title_run, reference, title_reference, setup=setup)
        flags = json.loads((run/'build-receipt.json').read_text())['flags']
        require('-DPOP2_INTEGRATION_TITLE_STATE' not in flags, 'NATURAL ROUTE BUILD')
        reached = re.findall(r'^INTEGRATION TITLE phase=(\d+) cue=(\d+)$', log, re.M)
        require(len(reached)==1 and int(reached[0][0])>=3 and reached[0][1]=='114', 'NATURAL TITLE CUE')
    require(re.findall(r'^INTEGRATION TITLE phase=(\d+) cue=(\d+)$', title_log, re.M)==[('16','114')], 'PAIRED TITLE FRAME AND CUE')
    for label in ('title', 'first-room'):
        source = title_source if label=='title' else run
        require(crop((source/(label+'-screen.bin')).read_bytes()) ==
                crop(title['indices'] if label=='title' else (reference/(label+'.indices')).read_bytes()), label+' PIXELS')
        # Color-table seed is bookkeeping; compare all flags, indices and RGB words.
        colors = (source/(label+'-colors.bin')).read_bytes()
        require(len(colors) == 2056 and colors[4:] ==
                (title['clut'] if label=='title' else (reference/(label+'.clut')).read_bytes())[4:], label+' COLORS')
    expected = crop((reference/'first-room.indices').read_bytes())
    require(decode_planes((run/'first-room-planes.bin').read_bytes()) == expected, 'AGA PLANES')
    display = re.findall(r'^INTEGRATION DISPLAY planes=([0-9a-f]+) copper=([0-9a-f]+) parity=(\d+) frames=(\d+)$', log, re.M)
    require(len(display) == 1 and int(display[0][3]) > 0, 'DISPLAY PUBLICATION')
    base, _, parity, _ = display[0]
    require(decode_copper((run/'first-room-copper.bin').read_bytes(), int(base,16), int(parity)) ==
            (reference/'first-room.rgb').read_bytes(), 'AGA PALETTE AND MODE')
    native = (run/'first-room-a5.bin').read_bytes()
    original = (reference/'first-room-a5.bin').read_bytes()
    require(len(native) == len(original) == 22266, 'A5 EXTENT')
    require(native[-0x18fc:-0x18f8] == original[-0x18fc:-0x18f8], 'FIRST ROOM RNG')
    native = (run/'first-room-context.bin').read_bytes()
    original = (reference/'first-room-context.bin').read_bytes()
    require(len(native) == len(original) == 0x2200, 'CONTEXT EXTENT')
    require(native[0x2180:0x21ba] == original[0x2180:0x21ba], 'FIRST ROOM CONTEXT WORDS')
    print(('INTEGRATION PASS' if right_expected else 'INTEGRATION NO-RIGHT CONTROL PASS') +
          ': native title and first room, reference pixels/state, input reads, route completion and restoration')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('title_reference', type=Path, help='state-paired original title renderer fixture')
    parser.add_argument('--title-run', type=Path, help='separate state-paired native title fixture')
    parser.add_argument('--no-right', action='store_true', help='require the completed no-input control route')
    parser.add_argument('--setup', choices=['a1200-fast','a1200-030','a1200-040'], default='a1200-fast')
    args = parser.parse_args()
    check(args.run, args.reference, args.title_reference, right_expected=not args.no_right, setup=args.setup, title_run=args.title_run)
