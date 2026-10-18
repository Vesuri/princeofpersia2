"""Qualify persistent opponent snapshots; retain the failed strict route."""
import hashlib
import json
import re
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


def timer_rng(original, native):
    observed = []
    for text, is_original in ((original, True), (native, False)):
        prefix = 'MAP seg=' if is_original else 'ROUTE_RANDOM_MAP segment='
        maps = {}
        for segment, base in re.findall(r'^'+prefix+r'(\d+) base=(\w+)', text, re.M):
            segment, base = int(segment), int(base, 16)
            if base and segment in (4, 5, 8):
                assert segment not in maps or maps[segment] == base, 'OPPONENT RNG / STABLE MAP'
                maps[segment] = base
        rows = [tuple(int(value, 10 if i == 0 else 16) for i, value in enumerate(row))
                for row in re.findall(r'^ROUTE_RANDOM step=(\d+) seed=(\w+) result=(\w+) site=(\w+) caller=(\w+)', text, re.M)]
        prefix_rows = []
        for row in rows[:110]:
            owners = [(segment, offset) for segment, offset in ((4, 0x1b6a), (5, 0x50ba), (8, 0x3b5c), (5, 0x205e))
                      if row[4] == maps[segment]+offset]
            assert len(owners) == 1, 'OPPONENT RNG / PREFIX CALLER'
            prefix_rows.append((*row[:4], owners[0]))
        timers = [(step, result) for step, _, result, _, caller in rows
                  if caller == maps[4]+0x1b6a and step <= 1319]
        observed.append((rows, prefix_rows, timers))
    a, b = observed
    assert (len(a[0]), len(b[0])) == (2957, 2952), 'OPPONENT RNG / COMPLETE STREAMS'
    assert all(x[1:4] == y[1:4] for x, y in zip(a[0], b[0])), 'OPPONENT RNG / IDENTICAL DRAW SERIES'
    assert a[1][:109] == b[1][:109], 'OPPONENT RNG / COMMON PREFIX'
    assert a[1][109][0] == b[1][109][0] == 67, 'OPPONENT RNG / FIRST SHIFT FRAME'
    assert (a[1][109][-1], b[1][109][-1]) == ((5, 0x50ba), (8, 0x3b5c)), 'OPPONENT RNG / AMBIENT VERSUS TORCH'
    assert a[2] == [(1290, 52257)] and b[2] == [(1290, 43250), (1311, 23607)], 'OPPONENT RNG / TIMER DRAWS'
    # Original CODE 4+1B62..1B78 chooses Random(30)+15, then +1704
    # decrements the persistent timer once per update. No native timer model.
    original_initial = a[2][0][1] % 31 + 15
    native_initial = b[2][0][1] % 31 + 15
    native_reset = b[2][1][1] % 31 + 15
    assert (original_initial, native_initial, native_reset) == (37, 20, 31), 'OPPONENT RNG / RANDOMIZED TIMERS'
    assert original_initial-(1319-1290) == 8 and native_reset-(1319-1311) == 23, 'OPPONENT RNG / CAPTURED TIMERS'
    assert 1290+native_initial+1 == 1311, 'OPPONENT RNG / RESET AFTER EXPIRY'
    return dict(identical_draw_prefix=2952, first_caller_shift_step=67,
                original_initial_timer=37, native_initial_timer=20,
                native_reset_step=1311, native_reset_timer=31,
                original_timer_at_1319=8, native_timer_at_1319=23)


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
    original_log = (original/'session.log').read_text()
    rng = timer_rng(original_log, log)
    for changed in (log.replace('step=1290 ', 'step=1291 '),
                    log.replace('result=A8F2 ', 'result=0000 ')):
        assert changed != log, 'OPPONENT RNG / EFFECTIVE CONTROL'
        try:
            timer_rng(original_log, changed)
        except AssertionError:
            pass
        else:
            raise AssertionError('OPPONENT RNG / CORRUPTION ACCEPTED')
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
                  original_repeat_matches=True, timer_rng=rng,
                  rng_corruption_controls=2, whole_route_accepted=False)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    check()
