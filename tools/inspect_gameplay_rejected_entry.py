"""Compare a rejected final entry without accepting the failed gameplay route."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from check_core_result import restored
from check_display_scenes import decode_copper
from check_gameplay_opening import compare_capture
from check_gameplay_restart import paired_frame
from gameplay_assisted_inputs import metadata


def inspect(native, reference, manifest, *, terminal_rng=None):
    _, points, _, labels = metadata(reference, manifest)
    log = (native / 'gdb-out.log').read_text()
    assert log.count('CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n') == 1, 'ENTRY / FAILURE OWNER'
    assert re.search(r'^ROUTE RESULT complete=0 ', log, re.M), 'ENTRY / MUST REMAIN FAILED'
    assert 'INTEGRATION AUDIT code=1 input=1\n' in log, 'ENTRY / AUDIT'
    restored(native, log, 'a1200-030')
    receipt = json.loads((native / 'build-receipt.json').read_text())
    assert {'-DPOP2_VERIFY','-DPOP2_GAMEPLAY_ROUTE_PROBE','-DPOP2_GAMEPLAY_ROUTE_LEVEL=0'} <= set(receipt['flags'].split()), 'ENTRY / BUILD FLAGS'
    for key, name in (('exe','dh1/PrinceOfPersia2'),('elf','PrinceOfPersia2.elf')):
        assert hashlib.sha256((native / name).read_bytes()).hexdigest() == receipt[key], 'ENTRY / BUILD IDENTITY'
    rows = re.findall(r'^ROUTE CHECKPOINT ordinal=(\d+) frames=(\d+) keys=(\d+) level=(\d+) room=(\d+) step=(\d+) planes=([0-9a-f]+) parity=(\d+)$', log, re.M)
    assert len(rows) == len(points)-1 and points[-1][4], 'ENTRY / FINAL CHECKPOINT REQUIRED'
    for ordinal, row in enumerate(rows):
        expected = points[ordinal]
        assert tuple(map(int,row[:6])) == (ordinal,expected[0],expected[3],expected[1],expected[2],expected[0]), 'ENTRY / PRECEDING COUNTERS'
        label = f'route-{ordinal}'
        paired_frame(native, reference, label, labels[ordinal])
        assert (native / (label+'-context.bin')).read_bytes() == (reference / (labels[ordinal]+'-context.bin')).read_bytes(), 'ENTRY / PRECEDING CONTEXT'
        actual = (native / (label+'-a5.bin')).read_bytes()
        original = (reference / (labels[ordinal]+'-a5.bin')).read_bytes()
        assert actual[-0x5208:-0x5208+372] == original[-0x5208:-0x5208+372], 'ENTRY / PRECEDING ACTORS'
        assert decode_copper((native / (label+'-copper.bin')).read_bytes(),int(row[6],16),int(row[7])) == (reference / (labels[ordinal]+'.rgb')).read_bytes(), 'ENTRY / PRECEDING COPPER'
    entries = re.findall(r'^ROUTE REJECTED ENTRY frames=(\d+) keys=(\d+) level=(\d+) room=(\d+) step=(\d+)$',log,re.M)
    assert len(entries) == 1, 'ENTRY / REJECTED CAPTURE'
    frames, keys, level, room, step = map(int,entries[0])
    final = points[-1]
    assert level == final[1] and room == final[2] and frames == step, 'ENTRY / DESTINATION'
    label = 'route-rejected-entry'
    compare_capture((native / (label+'-screen.bin')).read_bytes(),
                    (native / (label+'-colors.bin')).read_bytes(),
                    (native / 'dh1/video-transfer.bin').read_bytes(),
                    (reference / (labels[-1]+'.indices')).read_bytes(),
                    (reference / (labels[-1]+'.rgb')).read_bytes())
    assert (native / (label+'-context.bin')).read_bytes() == (reference / (labels[-1]+'-context.bin')).read_bytes(), 'ENTRY / FINAL CONTEXT'
    actual = (native / (label+'-a5.bin')).read_bytes()
    original = (reference / (labels[-1]+'-a5.bin')).read_bytes()
    assert len(actual) == len(original) == 22266, 'ENTRY / A5 EXTENT'
    for distance, size in ((0x5208,372),(0x18fc,4),(0x4fd2,2),(0x4fc2,2)):
        if distance == 0x18fc and terminal_rng is not None:
            assert level == 9 and room == 10, 'ENTRY / TERMINAL RNG SCOPE'
            assert int.from_bytes(actual[-distance:-distance+size], 'big') == terminal_rng['native_seed'], 'ENTRY / VERIFIED NATIVE RNG'
            assert int.from_bytes(original[-distance:-distance+size], 'big') == terminal_rng['original_seed'], 'ENTRY / VERIFIED ORIGINAL RNG'
            assert frames == terminal_rng['native_transition'] and final[0] == terminal_rng['original_transition'], 'ENTRY / VERIFIED RNG TIMING'
        else:
            assert actual[-distance:-distance+size] == original[-distance:-distance+size], 'ENTRY / FINAL GAME STATE'
    assert int.from_bytes(actual[-0x349a:-0x3498],'big') == step, 'ENTRY / ACTUAL STEP'
    assert int.from_bytes(original[-0x349a:-0x3498],'big') == final[0], 'ENTRY / ORIGINAL STEP'
    report = dict(route_accepted=False, preceding_checkpoints=len(rows),
                  next_level=level, next_room=room, native_frames=frames,
                  original_frames=final[0], native_keys=keys, original_keys=final[3],
                  endpoint_pixels_palette_context_actors_rng_match=terminal_rng is None)
    if terminal_rng is not None:
        report['verified_terminal_rng_difference'] = terminal_rng
    (native / 'rejected-entry-comparison.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
    return report


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path);parser.add_argument('reference',type=Path)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args();inspect(args.native,args.reference,args.manifest)
