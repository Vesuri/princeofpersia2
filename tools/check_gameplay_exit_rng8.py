"""Pair the extra level-eight boundary ambient draw without accepting strict timing."""
import hashlib
import json
from pathlib import Path

from check_gameplay_exit_audit import ROOT, exit_rows, rng_records, state_differences, verify_manifest
from check_core_result import restored
from replay_gameplay_route import compare


def calls(original, native, original_seed, native_seed):
    a, b = rng_records(original, True), rng_records(native, False)
    assert a == [(1193, 8, native_seed, native_seed & 0xffff, 0x6e1e, (5, 0x50ba)),
                 (1193, 9, original_seed, original_seed & 0xffff, 0x6e1e, (5, 0x50ba))], 'EXIT RNG8 / ORIGINAL CALLS'
    assert b == [(1190, 9, native_seed, native_seed & 0xffff, 0x6e1e, (5, 0x50ba))], 'EXIT RNG8 / NATIVE CALLS'
    first = original.index('EXIT_RNG step=04A9 level=0008 ')
    transition = original.index('ROUTE_EXIT site=61CE step=04A9 ')
    second = original.index('EXIT_RNG step=04A9 level=0009 ')
    assert first < transition < second, 'EXIT RNG8 / ORIGINAL TRANSITION ORDER'
    assert native.index('ROUTE_EXIT site=61CE step=1190 ') < native.index('EXIT_RNG step=04A6 level=0009 '), 'EXIT RNG8 / NATIVE TRANSITION ORDER'


def check():
    runs = {}
    for side, logname in (('original','session.log'), ('native','gdb-out.log')):
        recorded = verify_manifest(ROOT/f'manifest/gameplay-item16-exit-rng8-{side}.sha256')
        candidates = [Path(name).parent for name in recorded if Path(name).name == logname]
        assert len(candidates) == 1, 'EXIT RNG8 / RUN IDENTITY'
        runs[side] = ROOT/candidates[0]
    original, native = runs['original'], runs['native']
    reference = ROOT/'ref/mame/runs/gameplay-level8-direct-stairs-1'
    baseline = ROOT/'amiga/.run-a9ls3Q'
    verify_manifest(ROOT/'manifest/gameplay-assisted-level8-direct-stairs.sha256', reference)
    verify_manifest(ROOT/'manifest/gameplay-item16-exit-audit-8-native.sha256')
    for seq in range(len((reference/'route-actions.txt').read_text().splitlines())+1):
        compare(original, reference, seq)
    a, b = (original/'session.log').read_text(), (native/'gdb-out.log').read_text()
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in a, 'EXIT RNG8 / ORIGINAL COMPLETION'
    assert 'ROUTE RESTORED\n' in b and 'INTEGRATION AUDIT code=1 input=1\n' in b, 'EXIT RNG8 / NATIVE AUDIT'
    assert 'CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n' in b, 'EXIT RNG8 / STRICT FAILURE RETAINED'
    restored(native, b, 'a1200-030')
    receipt = json.loads((native/'build-receipt.json').read_text())
    assert receipt == json.loads((baseline/'build-receipt.json').read_text()), 'EXIT RNG8 / BASELINE BUILD'
    for key, name in (('exe','dh1/PrinceOfPersia2'), ('elf','PrinceOfPersia2.elf')):
        assert hashlib.sha256((native/name).read_bytes()).hexdigest() == receipt[key], 'EXIT RNG8 / BUILD IDENTITY'
    assert not state_differences(native, baseline), 'EXIT RNG8 / UNCHANGED NATIVE ROUTE'
    ga, gb = exit_rows(a, 8, True), exit_rows(b, 8, False)
    assert ga['pending_start'] == gb['pending_start'] == 981, 'EXIT RNG8 / PENDING START'
    assert (ga['transition'], gb['transition']) == (1193,1190), 'EXIT RNG8 / TRANSITIONS'
    def seed(run, label):
        return int.from_bytes((run/f'{label}-a5.bin').read_bytes()[-0x18fc:-0x18f8], 'big')
    assert seed(original,'route-48') == seed(native,'route-76'), 'EXIT RNG8 / COMMON PRECEDING SEED'
    original_seed, native_seed = seed(original,'route-49'), seed(native,'route-rejected-entry')
    assert original_seed != native_seed, 'EXIT RNG8 / DISTINCT ENTRY SEEDS'
    calls(a,b,original_seed,native_seed)
    for changed_a, changed_b in ((a.replace('EXIT_RNG step=04A9 level=0008 ', 'MISSING step=04A9 level=0008 '), b),
                                 (a, b.replace(f'seed={native_seed:08X}', 'seed=00000000'))):
        assert changed_a != a or changed_b != b, 'EXIT RNG8 / EFFECTIVE CONTROL'
        try:calls(changed_a,changed_b,original_seed,native_seed)
        except AssertionError:pass
        else:raise AssertionError('EXIT RNG8 / CORRUPTION ACCEPTED')
    result = dict(original_seed=original_seed, native_seed=native_seed,
                  extra_original_ambient_draws=1, original_transition=1193,
                  native_transition=1190, corruption_controls=2)
    (ROOT/'tmp/item16-exit-rng8-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
    return result


if __name__ == '__main__':
    print(json.dumps(check(),sort_keys=True))
