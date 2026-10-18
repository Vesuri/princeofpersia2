"""Check the bounded level-three RNG-call divergence, not the whole route."""
import hashlib
import json
import re
from collections import Counter

from extract_original_data import ROOT, pins


def rows(log):
    result = []
    for step, seed, value, site, caller in re.findall(
            r'^ROUTE_RANDOM step=(\d+) seed=(\w+) result=(\w+) site=(\w+) caller=(\w+)$', log, re.M):
        step = int(step)
        if 1490 <= step <= 1545:
            result.append((step, int(seed, 16), int(value, 16), int(site, 16), int(caller, 16)))
    return result


def compare(original_log, native_log):
    original, native = rows(original_log), rows(native_log)
    assert len(original) == 165 and len(native) == 164, 'RNG WINDOW / CALL COUNTS'
    assert original[0][0] == native[0][0] == 1490, 'RNG WINDOW / START'
    assert original[-1][0] == native[-1][0] == 1545, 'RNG WINDOW / END'
    maps = {int(s): (int(b, 16), int(n)) for s, b, n in re.findall(
        r'^MAP seg=(\d+) base=([0-9A-F]+) size=(\d+)$', original_log, re.M)}
    def original_caller(address):
        owners = [(segment, address-base) for segment, (base, size) in maps.items()
                  if base <= address < base+size]
        assert len(owners) == 1, 'RNG WINDOW / ORIGINAL CALLER OWNER'
        return owners[0]
    original_kinds = [original_caller(row[4]) for row in original]
    assert Counter(original_kinds) == {(8, 0x3b5c): 161, (5, 0x205e): 3,
                                       (5, 0x50ba): 1}, 'RNG WINDOW / ORIGINAL CALLERS'
    ambient = original_kinds.index((5, 0x50ba))
    assert original[ambient][0] == 1515, 'RNG WINDOW / AMBIENT STEP'
    # This class correspondence is inferred from identical per-frame call order
    # and multiplicity. The older native observer did not dump its segment map.
    counts = Counter(row[4] for row in native)
    assert sorted(counts.values()) == [3, 161], 'RNG WINDOW / NATIVE CALLER CLASSES'
    classes = {caller: (8, 0x3b5c) if count == 161 else (5, 0x205e)
               for caller, count in counts.items()}
    expected = [(row[0], kind, row[3]) for row, kind in zip(original, original_kinds)
                if kind != (5, 0x50ba)]
    actual = [(row[0], classes[row[4]], row[3]) for row in native]
    assert expected == actual, 'RNG WINDOW / FRAME AND CALL ORDER'
    assert all(a[1:3] == b[1:3] for a, b in zip(original, native)), 'RNG WINDOW / SEED SERIES'
    assert original[ambient][1:3] != original[ambient+1][1:3], 'RNG WINDOW / DISTINCT DRAW'
    return ambient


def main():
    for name, digest in pins(ROOT/'manifest/gameplay-item16-random-window.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'RNG WINDOW / PIN '+name
    original = (ROOT/'ref/mame/runs/gameplay-level3-random-1/session.log').read_text()
    run = ROOT/'amiga/.run-845iXe'
    receipt = json.loads((run/'build-receipt.json').read_text())
    assert {'-DPOP2_VERIFY', '-DPOP2_ROUTE_RANDOM_TRACE'} <= set(receipt['flags'].split()), 'RNG WINDOW / BUILD FLAGS'
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        assert hashlib.sha256((run/name).read_bytes()).hexdigest() == receipt[key], 'RNG WINDOW / BUILD IDENTITY'
    native = (run/'gdb-out.log').read_text()
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in original, 'RNG WINDOW / ORIGINAL INCOMPLETE'
    assert 'INTEGRATION AUDIT code=1 input=1\n' in native and 'ROUTE RESTORED\n' in native, 'RNG WINDOW / NATIVE AUDIT'
    assert 'CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n' in native, 'RNG WINDOW / RETAIN FAILURE'
    index = compare(original, native)
    selected = rows(native)[index]
    for changed in (native.replace(f'seed={selected[1]:08x}', 'seed=00000000'),
                    native.replace('step=1545 ', 'step=1546 ')):
        try:
            compare(original, changed)
        except AssertionError:
            continue
        raise AssertionError('RNG WINDOW / CORRUPTION CONTROL ACCEPTED')
    print('RNG WINDOW PASS: identical 164-draw seed/result prefix; one extra original '
          'PlayAmbient draw at step 1515; matching remaining per-frame call classes; '
          'two corruption controls. Native caller classification is inferred; route remains failed.')


if __name__ == '__main__':
    main()
