"""Qualify each observed exit branch; keep strict route and state differences visible."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from extract_original_data import ROOT, pins
from check_core_result import restored
from replay_gameplay_route import compare


def exit_rows(log, level, original):
    rows = []
    for line in log.splitlines():
        if not line.startswith('ROUTE_EXIT '):
            continue
        match = re.fullmatch(r'ROUTE_EXIT site=(\w+) step=(\w+) level=(\w+) next=(\w+) cue=(\w+) result=(\w+)' + (r' tick=(\w+)' if original else ''), line)
        assert match, 'EXIT AUDIT / MALFORMED ROW'
        values = tuple(int(v, 16 if original or i not in (1, 2, 3) else 10) for i, v in enumerate(match.groups()[:6]))
        if values[2] == level:
            rows.append(values)
    assert rows, 'EXIT AUDIT / TARGET LEVEL NOT OBSERVED'
    start = rows[0][1]
    cursor = 0
    step = start
    blocked = []
    while cursor < len(rows):
        request = rows[cursor]
        assert request[:4] == (0x6134, step, level, level+1), 'EXIT AUDIT / REQUEST ORDER'
        cursor += 1
        assert cursor < len(rows), 'EXIT AUDIT / MISSING FIRST SOUND RESULT'
        first = rows[cursor]
        assert first[:4] == (0x6144, step, level, level+1), 'EXIT AUDIT / FIRST SOUND ORDER'
        cursor += 1
        if first[5]:
            blocked.append((step, 'first-sound', first[4], first[5]))
            step += 1
            continue
        assert cursor < len(rows), 'EXIT AUDIT / MISSING SECOND SOUND RESULT'
        second = rows[cursor]
        assert second[:4] == (0x6152, step, level, level+1), 'EXIT AUDIT / SECOND SOUND ORDER'
        cursor += 1
        if second[5] and second[4] not in (0x110, 0x11, 0xc0):
            blocked.append((step, 'current-cue', second[4], second[5]))
            step += 1
            continue
        assert cursor < len(rows), 'EXIT AUDIT / MISSING TRANSITION'
        transition = rows[cursor]
        assert transition[:4] == (0x61ce, step, level, level+1), 'EXIT AUDIT / TRANSITION ORDER'
        cursor += 1
        assert cursor == len(rows), 'EXIT AUDIT / AFTER TRANSITION'
        return dict(pending_start=start, transition=step, waiting_frames=step-start,
                    blocks=blocked, final_cue=second[4], final_sound_result=second[5])
    raise AssertionError('EXIT AUDIT / TRANSITION NOT REACHED')


def rng_records(log, is_original):
    pattern = (r'^MAP seg=(\d+) base=([0-9A-F]+) size=(\d+)$' if is_original
               else r'^EXIT_RNG_MAP segment=(\d+) base=([0-9A-F]+) size=(\d+)$')
    maps = {int(seg): (int(base, 16), int(size))
            for seg, base, size in re.findall(pattern, log, re.M)}
    result = []
    for row in re.findall(r'^EXIT_RNG step=(\w+) level=(\w+) seed=(\w+) result=(\w+) site=(\w+) caller=(\w+)$', log, re.M):
        step, level, seed, value, site, caller = map(lambda v: int(v, 16), row)
        owners = [(seg, caller-base) for seg, (base, size) in maps.items()
                  if base <= caller < base+size]
        assert len(owners) == 1, 'EXIT RNG / CALLER OWNER'
        result.append((step, level, seed, value, site, owners[0]))
    return result


def terminal_rng(original, native, original_gate, native_gate, original_seed, native_seed):
    """Account for every extra final-level draw with observed original callers."""
    def decode(log, is_original):
        records = rng_records(log, is_original)
        assert records and records[0][0] == 1490, 'EXIT RNG / START'
        assert all(row[1] == 13 and row[4] == 0x6e1e for row in records), 'EXIT RNG / LEVEL AND SITE'
        return [(row[0], row[2], row[3], row[5]) for row in records]
    a, b = decode(original, True), decode(native, False)
    assert a[:len(b)] == b, 'EXIT RNG / MATCHED WAITING FRAMES'
    assert a[-1][0] == original_gate['transition'], 'EXIT RNG / ORIGINAL END'
    assert b[-1][0] == native_gate['transition'], 'EXIT RNG / NATIVE END'
    extra = a[len(b):]
    assert len(extra) == 5, 'EXIT RNG / EXTRA DRAWS'
    assert [r[0] for r in extra] == [1516,1516,1517,1517,1517], 'EXIT RNG / EXTRA FRAMES'
    assert [r[3] for r in extra] == [(10,0x12c6),(10,0x12c6),(5,0x50ba),(10,0x12c6),(10,0x12c6)], 'EXIT RNG / EXTRA CALLERS'
    assert b[-1][1] == native_seed and a[-1][1] == original_seed and native_seed != original_seed, 'EXIT RNG / ENTRY SEEDS'
    return dict(matching_draws=len(b), extra_draws=len(extra),
                extra_torch_draws=4, extra_ambient_draws=1,
                native_entry_seed=f'{b[-1][1]:08X}', original_entry_seed=f'{a[-1][1]:08X}')


def case_for(level):
    path = ROOT/'tmp/item12-exceptions.json'
    assert hashlib.sha256(path.read_bytes()).hexdigest() == pins(ROOT/'manifest/gameplay-closure.sha256')['tmp/item12-exceptions.json'], 'EXIT AUDIT / CASE INDEX PIN'
    return next(c for c in json.loads(path.read_text()) if c['level'] == level)


def verify_manifest(path, root=ROOT):
    recorded = pins(path)
    for name, digest in recorded.items():
        assert hashlib.sha256((root/name).read_bytes()).hexdigest() == digest, 'EXIT AUDIT / PIN '+name
    return recorded


def state_differences(native, baseline):
    differences = []
    labels = sorted(p.name.removesuffix('-a5.bin') for p in baseline.glob('route-*-a5.bin'))
    assert labels == sorted(p.name.removesuffix('-a5.bin') for p in native.glob('route-*-a5.bin')), 'EXIT AUDIT / BASELINE CAPTURE COVERAGE'
    for label in labels:
        x, y = (native/f'{label}-a5.bin').read_bytes(), (baseline/f'{label}-a5.bin').read_bytes()
        assert len(x) == len(y) == 22266, 'EXIT AUDIT / A5 CAPTURE EXTENT'
        fields = []
        for name, offset, size in (('actors',0x5208,372),('rng',0x18fc,4),('level',0x4fd2,2),('room',0x4fc2,2),('step',0x349a,2)):
            if x[-offset:-offset+size] != y[-offset:-offset+size]:fields.append(name)
        for name in ('screen', 'context', 'colors', 'planes'):
            xfile, yfile = native/f'{label}-{name}.bin', baseline/f'{label}-{name}.bin'
            if name == 'planes' and label == 'route-rejected-entry':
                assert xfile.exists() == yfile.exists(), 'EXIT AUDIT / PLANE CAPTURE COVERAGE'
                if not xfile.exists():continue
            if xfile.read_bytes() != yfile.read_bytes():fields.append(name)
        if fields:differences.append(dict(checkpoint=label,fields=fields))
    return differences


def check_level4():
    """Add original exit branches to the already pinned timer/actor causal pair."""
    from check_gameplay_opponent_state import check as check_opponents
    check_opponents()
    recorded = verify_manifest(ROOT/'manifest/gameplay-item16-exit4-original.sha256')
    logs = [ROOT/name for name in recorded if Path(name).name == 'session.log']
    assert len(logs) == 1, 'EXIT4 / ORIGINAL IDENTITY'
    original = logs[0].parent
    native = ROOT/'amiga/.run-KkYYZ2'
    reference = ROOT/'ref/mame/runs/gameplay-assisted-level4-1'
    verify_manifest(ROOT/'manifest/gameplay-assisted-level4.sha256', reference)
    for seq in range(len((reference/'route-actions.txt').read_text().splitlines())+1):
        compare(original, reference, seq)
    a, b = logs[0].read_text(), (native/'gdb-out.log').read_text()
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in a, 'EXIT4 / ORIGINAL COMPLETION'
    pattern = r'^GAMEPLAY_ROUTE_KEYS frame=\d+ step=\d+ map=[0-9A-F]{32}$'
    assert re.findall(pattern,a,re.M) == re.findall(pattern,(reference/'session.log').read_text(),re.M), 'EXIT4 / CONSUMED INPUT'
    ga, gb = exit_rows(a,4,True), exit_rows(b,4,False)
    assert ga['pending_start'] == gb['pending_start'] == 1817, 'EXIT4 / PENDING START'
    assert (ga['transition'],gb['transition']) == (1889,1888), 'EXIT4 / TRANSITION'
    for log, is_original in ((a,True),(b,False)):
        altered = log.replace('ROUTE_EXIT site=61CE','MISSING site=61CE')
        assert altered != log, 'EXIT4 / EFFECTIVE CONTROL'
        try:exit_rows(altered,4,is_original)
        except AssertionError:pass
        else:raise AssertionError('EXIT4 / MISSING TRANSITION ACCEPTED')
    result = dict(level=4, original=str(original.relative_to(ROOT)), native=str(native.relative_to(ROOT)),
                  original_gate=ga, native_gate=gb, original_repeat_matches=True,
                  existing_opponent_trace_verified=True, missing_transition_controls=2,
                  whole_route_accepted=False)
    (ROOT/'tmp/item16-exit-audit-level4-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('original_gate','native_gate')},sort_keys=True))


def check(level, native_manifest=None, first_level_control=False):
    if level == 4:
        assert native_manifest is None and not first_level_control, 'EXIT4 / FIXED CAUSAL EVIDENCE'
        return check_level4()
    assert not first_level_control or level == 3, 'EXIT AUDIT / FIRST-LEVEL CONTROL SCOPE'
    observed_level = 1 if first_level_control else level
    case = case_for(level)
    def evidence_manifest(side):
        return native_manifest if side == 'native' and native_manifest else ROOT/f'manifest/gameplay-item16-exit-audit-{level}-{side}.sha256'
    def observed(side):
        recorded = pins(evidence_manifest(side))
        logname = 'session.log' if side == 'original' else 'gdb-out.log'
        selected = [Path(name).parent for name in recorded if Path(name).name == logname]
        assert len(selected) == 1, 'EXIT AUDIT / UNIQUE OBSERVED RUN'
        return ROOT/selected[0]
    original, native = observed('original'), observed('native')
    for side, run, logname in (('original', original, 'session.log'), ('native', native, 'gdb-out.log')):
        recorded = verify_manifest(evidence_manifest(side))
        assert str((run/logname).relative_to(ROOT)) in recorded, 'EXIT AUDIT / RUN IDENTITY'
    reference, baseline = ROOT/case['reference'], ROOT/case['native']
    verify_manifest(ROOT/case['original_manifest'], reference)
    verify_manifest(ROOT/case['native_manifest'], baseline)
    if first_level_control:
        baseline = ROOT/'amiga/.run-NeChbT'
        verify_manifest(ROOT/'manifest/gameplay-item16-level1-baseline.sha256')
    for seq in range(len((reference/'route-actions.txt').read_text().splitlines())+1):
        compare(original, reference, seq)
    a, b = (original/'session.log').read_text(), (native/'gdb-out.log').read_text()
    assert a.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'EXIT AUDIT / ORIGINAL COMPLETION'
    key_pattern = r'^GAMEPLAY_ROUTE_KEYS frame=\d+ step=\d+ map=[0-9A-F]{32}$'
    assert re.findall(key_pattern, a, re.M) == re.findall(key_pattern, (reference/'session.log').read_text(), re.M), 'EXIT AUDIT / ORIGINAL CONSUMED INPUT'
    assert 'ROUTE RESTORED\n' in b and 'INTEGRATION AUDIT code=1 input=1\n' in b, 'EXIT AUDIT / NATIVE AUDIT'
    restored(native, b, 'a1200-030')
    receipt = json.loads((native/'build-receipt.json').read_text())
    flags = receipt['flags'].split()
    required = {'-DPOP2_VERIFY', '-DPOP2_CORE', '-DPOP2_DISPLAY', '-DPOP2_INPUT',
                '-DPOP2_UI', '-DPOP2_AUDIO', '-DPOP2_INTEGRATION_PROBE',
                '-DPOP2_GAMEPLAY_ROUTE_PROBE',
                '-DPOP2_GAMEPLAY_ROUTE_LEVEL='+('3' if level == 3 else '0')}
    if level == 3 and '-DPOP2_ROUTE_NO_TRAP_TRACE' in flags:
        required.add('-DPOP2_ROUTE_NO_TRAP_TRACE')
    assert set(f for f in flags if f.startswith('-DPOP2_')) == required, 'EXIT AUDIT / DIAGNOSTIC FLAGS'
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        assert hashlib.sha256((native/name).read_bytes()).hexdigest() == receipt[key], 'EXIT AUDIT / BUILD IDENTITY'
    historical_build = level == 3 and receipt == json.loads((ROOT/case['native']/'build-receipt.json').read_text())
    if historical_build:
        legacy = verify_manifest(ROOT/'manifest/gameplay-item16-legacy-inputs.sha256')
        expected = legacy['amiga/.run-Vz4heJ/dh1/relocations.bin']
        assert hashlib.sha256((native/'dh1/relocations.bin').read_bytes()).hexdigest() == expected, 'EXIT AUDIT / HISTORICAL INPUT PLAN'
    terminal_rng8 = None
    if level == 8:
        from check_gameplay_exit_rng8 import check as check_rng8
        terminal_rng8 = check_rng8()
    if level in (7,8,10,11,12):
        from inspect_gameplay_rejected_entry import inspect
        inspect(native, reference, ROOT/case['original_manifest'], terminal_rng=terminal_rng8)
    original_gate, native_gate = exit_rows(a, observed_level, True), exit_rows(b, observed_level, False)
    rng = None
    if level == 13:
        from gameplay_assisted_inputs import metadata
        labels = metadata(reference, ROOT/case['original_manifest'])[3]
        original_seed = int.from_bytes((original/(labels[-1]+'-a5.bin')).read_bytes()[-0x18fc:-0x18f8], 'big')
        native_seed = int.from_bytes((native/'route-rejected-entry-a5.bin').read_bytes()[-0x18fc:-0x18f8], 'big')
        rng = terminal_rng(a, b, original_gate, native_gate, original_seed, native_seed)
    if rng:
        changed = a.replace(f'seed={original_seed:08X}', 'seed=00000000')
        assert changed != a, 'EXIT RNG / EFFECTIVE CONTROL'
        try:terminal_rng(changed, b, original_gate, native_gate, original_seed, native_seed)
        except AssertionError:pass
        else:raise AssertionError('EXIT RNG / CHANGED FINAL SEED ACCEPTED')
    # Report every changed retained state; a verified branch does not waive it.
    differences = state_differences(native, baseline)
    baseline_difference_matches_original = None
    if historical_build:
        assert differences == [dict(checkpoint='route-90',fields=['rng'])], 'EXIT AUDIT / HISTORICAL BASELINE DIFFERENCES'
        captured = (native/'route-90-a5.bin').read_bytes()[-0x18fc:-0x18f8]
        expected = (original/'route-90-a5.bin').read_bytes()[-0x18fc:-0x18f8]
        assert captured == expected, 'EXIT AUDIT / EARLIER RNG MATCHES ORIGINAL'
        baseline_difference_matches_original = dict(checkpoint='route-90',fields=['rng'])
    causal_baseline = None
    if level == 13:
        causal_baseline = ROOT/'amiga/.run-NeBYwx'
        verify_manifest(ROOT/'manifest/gameplay-item16-shadow-gate-native.sha256')
        assert not state_differences(native, causal_baseline), 'EXIT AUDIT / SHADOW CAUSAL BASELINE'
    for log, is_original in ((a, True), (b, False)):
        changed = log.replace('ROUTE_EXIT site=61CE', 'MISSING site=61CE')
        assert changed != log, 'EXIT AUDIT / EFFECTIVE CONTROL'
        try:exit_rows(changed, observed_level, is_original)
        except AssertionError:pass
        else:raise AssertionError('EXIT AUDIT / MISSING TRANSITION ACCEPTED')
    result = dict(level=observed_level, requested_route_level=level, historical_build_replay=historical_build, original=str(original.relative_to(ROOT)), native=str(native.relative_to(ROOT)),
                  original_gate=original_gate, native_gate=native_gate,
                  same_pending_start=original_gate['pending_start']==native_gate['pending_start'],
                  transition_lead=original_gate['transition']-native_gate['transition'],
                  baseline_state_differences=differences, original_repeat_matches=True,
                  baseline_difference_matches_original=baseline_difference_matches_original,
                  causal_baseline=str(causal_baseline.relative_to(ROOT)) if causal_baseline else None,
                  missing_transition_controls=2, terminal_rng=rng or terminal_rng8, whole_route_accepted=False)
    suffix = '-first-level-control' if first_level_control else ''
    output=ROOT/f'tmp/item16-exit-audit-level{level}{suffix}-comparison.json'
    output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('original_gate','native_gate')},sort_keys=True))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('level',type=int,choices=(3,4,7,8,10,11,12,13))
    parser.add_argument('--native-manifest', type=Path)
    parser.add_argument('--first-level-control', action='store_true', help='For route 3, qualify only the observed earlier level-one boundary; never a level-three pass')
    args = parser.parse_args()
    check(args.level, args.native_manifest, args.first_level_control)
