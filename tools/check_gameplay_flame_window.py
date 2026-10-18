"""Check sacred-flame updates and resets without accepting a complete native route."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from extract_original_data import ROOT, pins
from replay_gameplay_route import compare
from gameplay_assisted_inputs import metadata
from check_core_result import restored
from check_gameplay_opening import compare_capture


def updates(log, original):
    rows = re.findall(r'^ROUTE_FLAME site=(\w+) step=(\w+) state=(\w+) next=(\w+) shape=(\w+) tile=(\w+)$', log, re.M)
    assert rows and len(rows) % 2 == 0, 'FLAME / PAIRED OBSERVATIONS'
    pairs, resets = [], []
    previous = None
    for index in range(0, len(rows), 2):
        a, b = [tuple(int(v, 10 if i == 1 and not original else 16) for i, v in enumerate(row))
                for row in rows[index:index+2]]
        assert a[0] == 0x2cc2 and b[0] == 0x2d48, 'FLAME / UPDATE SITES'
        assert a[1] == b[1] and a[2] == b[2] == a[5] == b[5], 'FLAME / UPDATE PAIR'
        assert a[4] and a[4] == b[4], 'FLAME / SHAPE PRESENT'
        state = a[2]
        expected = (state-1 if state & 15 else 0) if state & 128 else (state+1 if state < 11 else 0)
        assert b[3] == expected, 'FLAME / ORIGINAL RECURRENCE'
        if previous is not None and previous != state:
            resets.append(dict(step=a[1], previous=previous, reset=state))
        pairs.append((a[1], state, b[3]))
        previous = b[3]
    assert [x[0] for x in pairs] == list(range(28, 641)), 'FLAME / COMPLETE FRAME WINDOW'
    return pairs, resets


def check(native=None):
    for name, digest in pins(ROOT/'manifest/gameplay-item16-flame-original.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'FLAME / PIN '+name
    original = ROOT/'ref/mame/runs/item16-flame-1'
    canonical = ROOT/'ref/mame/runs/gameplay-assisted-level13-flame-1'
    _, checkpoints, _, labels = metadata(canonical, ROOT/'manifest/gameplay-assisted-level13.sha256')
    log = (original/'session.log').read_text()
    assert log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'FLAME / ORIGINAL COMPLETION'
    for sequence in range(28):
        compare(original, canonical, sequence)
    pairs, resets = updates(log, True)
    assert resets == [dict(step=229, previous=9, reset=133)], 'FLAME / ORIGINAL RESET'
    for changed in (log.replace('ROUTE_FLAME site=2D48', 'MISSING site=2D48', 1),
                    log.replace('next=0001 shape=', 'next=0002 shape=', 1)):
        try:
            updates(changed, True)
        except AssertionError:
            continue
        raise AssertionError('FLAME / CORRUPTION CONTROL ACCEPTED')
    report = dict(original_updates=len(pairs), original_resets=resets,
                  original_repeat_matches=True, corruption_controls=2,
                  whole_route_accepted=False)
    if native:
        recorded = pins(ROOT/'manifest/gameplay-item16-flame-native.sha256')
        assert str((native/'gdb-out.log').resolve().relative_to(ROOT)) in recorded, 'FLAME / UNPINNED NATIVE'
        for name, digest in recorded.items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'FLAME / NATIVE PIN '+name
        nlog = (native/'gdb-out.log').read_text()
        assert 'ROUTE RESTORED\n' in nlog and 'INTEGRATION AUDIT code=1 input=1\n' in nlog, 'FLAME / NATIVE COMPLETION AUDIT'
        restored(native, nlog, 'a1200-030')
        assert nlog.count('CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n') == 1, 'FLAME / RETAIN STRICT FAILURE'
        receipt = json.loads((native/'build-receipt.json').read_text())
        assert {'-DPOP2_VERIFY', '-DPOP2_GAMEPLAY_ROUTE_PROBE', '-DPOP2_GAMEPLAY_ROUTE_LEVEL=0'} <= set(receipt['flags'].split()), 'FLAME / BUILD FLAGS'
        for key, path in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
            assert hashlib.sha256((native/path).read_bytes()).hexdigest() == receipt[key], 'FLAME / BUILD IDENTITY'
        npairs, nresets = updates(nlog, False)
        assert nresets == [dict(step=228, previous=8, reset=133)], 'FLAME / NATIVE RESET'
        context_differences, pixel_differences = [], []
        for ordinal, label in enumerate(labels[:-1]):
            original_a5 = (canonical/(label+'-a5.bin')).read_bytes()
            native_a5 = (native/f'route-{ordinal}-a5.bin').read_bytes()
            assert len(original_a5) == len(native_a5) == 22266, 'FLAME / A5 EXTENT'
            for distance, size in ((0x5208, 372), (0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2), (0x349a, 2)):
                assert original_a5[-distance:-distance+size] == native_a5[-distance:-distance+size], 'FLAME / PRECEDING ACTORS RNG COUNTERS'
            a = (canonical/(label+'-context.bin')).read_bytes()
            b = (native/f'route-{ordinal}-context.bin').read_bytes()
            assert len(a) == len(b) == 0x2200, 'FLAME / CONTEXT EXTENT'
            different = [i for i, (x, y) in enumerate(zip(a, b)) if x != y]
            if different:
                assert different == [0x915], 'FLAME / OTHER CONTEXT DIFFERENCE'
                context_differences.append(ordinal)
            try:
                compare_capture((native/f'route-{ordinal}-screen.bin').read_bytes(),
                                (native/f'route-{ordinal}-colors.bin').read_bytes(),
                                (native/'dh1/video-transfer.bin').read_bytes(),
                                (canonical/(label+'.indices')).read_bytes(),
                                (canonical/(label+'.rgb')).read_bytes())
            except AssertionError as error:
                assert str(error) == 'OPENING / PIXELS', error
                pixel_differences.append(ordinal)
        assert context_differences == list(range(14, 36)) and pixel_differences == list(range(14, 25)), 'FLAME / CAPTURE DIFFERENCES'
        report.update(native_updates=len(npairs), native_resets=nresets,
                      first_state_difference=next((a[0] for a, b in zip(pairs, npairs) if a != b), None),
                      preceding_actors_rng_counters_match=True,
                      context_difference_ordinals=context_differences, pixel_difference_ordinals=pixel_differences)
    destination = native or original
    (destination/'flame-window-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path)
    check(parser.parse_args().native)
