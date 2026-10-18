"""Qualify persistent opponent snapshots; retain the failed strict route."""
import hashlib
import json
from extract_original_data import ROOT, pins
from gameplay_assisted_inputs import metadata
from replay_gameplay_route import compare
from check_core_result import restored


def records(blob):
    assert len(blob) == 0x1800, 'OPPONENTS / EXTENT'
    for room in range(32):
        assert int.from_bytes(blob[room*192:room*192+2], 'big') <= 5, 'OPPONENTS / COUNT'
    return blob


def differences(a, b):
    records(a); records(b)
    return [dict(room=i//192+1, slot=(i%192-2)//38, offset=(i%192-2)%38,
                 original=int.from_bytes(a[i:i+2], 'big'), native=int.from_bytes(b[i:i+2], 'big'))
            for i in range(0, len(a), 2) if a[i:i+2] != b[i:i+2]]


def check():
    original = ROOT/'ref/mame/runs/item16-level4-opponents-1'
    native = ROOT/'amiga/.run-KkYYZ2'
    canonical = ROOT/'ref/mame/runs/gameplay-assisted-level4-1'
    for side in ('original', 'native'):
        for name, digest in pins(ROOT/f'manifest/gameplay-item16-opponents-{side}.sha256').items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'OPPONENTS / PIN '+name
    _, points, _, labels = metadata(canonical, ROOT/'manifest/gameplay-assisted-level4.sha256')
    for sequence in range(91):
        compare(original, canonical, sequence)
    log = (native/'gdb-out.log').read_text()
    assert 'ROUTE RESTORED\n' in log and 'INTEGRATION AUDIT code=1 input=1\n' in log, 'OPPONENTS / AUDIT'
    assert log.count('CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n') == 1, 'OPPONENTS / STRICT FAILURE'
    assert 'ROUTE FIRST FRAME frames=1888 keys=1890 level=5 room=1 step=1888\n' in log, 'OPPONENTS / FAILED ENTRY'
    restored(native, log, 'a1200-030')
    results = []
    for ordinal, label in enumerate(labels[:-1]):
        a = (original/(label+'-opponents.bin')).read_bytes()
        b = (native/f'route-{ordinal}-opponents.bin').read_bytes()
        delta = differences(a, b)
        if delta:
            results.append(dict(ordinal=ordinal, step=points[ordinal][0], differences=delta))
    assert results[0] == dict(ordinal=75, step=1319, differences=[dict(room=27, slot=0, offset=30, original=8, native=23)]), 'OPPONENTS / FIRST DIFFERENCE'
    preceding = next(r for r in results if r['ordinal'] == 86)
    assert preceding['step'] == 1613 and len(preceding['differences']) == 5, 'OPPONENTS / ACTOR PREFIX'
    assert all(d['offset'] == 30 for d in preceding['differences']), 'OPPONENTS / TIMER ONLY'
    first_other = next(r for r in results if any(d['offset'] != 30 for d in r['differences']))
    assert (first_other['ordinal'], first_other['step']) == (87, 1673), 'OPPONENTS / FIRST NON TIMER'
    sample = (original/'route-83-opponents.bin').read_bytes()
    for corrupt in (sample[:-2], b'\x00\x06'+sample[2:]):
        try:
            records(corrupt)
        except AssertionError:
            continue
        raise AssertionError('OPPONENTS / CORRUPTION ACCEPTED')
    report = dict(first_difference=results[0], preceding_actor_checkpoint=preceding,
                  first_non_timer=first_other, corruption_controls=2,
                  original_repeat_matches=True, whole_route_accepted=False)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    check()
