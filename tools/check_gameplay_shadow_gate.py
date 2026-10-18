"""Pair death-sound completion and shadow creation without accepting route timing."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from extract_original_data import ROOT, pins
from replay_gameplay_route import compare
from check_core_result import restored


def gate(log):
    rows = [tuple(int(value, 10 if i == 0 else 16) for i, value in enumerate(row))
            for row in re.findall(r'^SHADOW_GATE segment=(\d+) site=(\w+) step=(\w+) result=(\w+) cue1=(\w+) cue2=(\w+)$', log, re.M)]
    waits = [row for row in rows if row[1] == 0x3c76]
    assert waits, 'SHADOW GATE / NO SOUND OBSERVATIONS'
    start, end = waits[0][2], waits[-1][2]
    assert start < end, 'SHADOW GATE / NO WAIT'
    expected = []
    for step in range(start, end+1):
        expected.extend(((4, 0x3c68, step, 0, 12, 188),
                         (4, 0x3c76, step, int(step != end), 12, 188)))
    expected.extend(((4, 0x3c96, end, 0, 12, 188),
                     (10, 0x2b14, end+1, 0xffff, 12, 188)))
    assert rows == expected, 'SHADOW GATE / COMPLETE BRANCH SEQUENCE'
    return dict(wait_start=start, sound_complete=end, shadow_creation=end+1)


def check(native):
    original = ROOT/'ref/mame/runs/item16-shadow-gate-1'
    canonical = ROOT/'ref/mame/runs/gameplay-assisted-level13-flame-1'
    for side in ('original', 'native'):
        recorded = pins(ROOT/f'manifest/gameplay-item16-shadow-gate-{side}.sha256')
        for name, digest in recorded.items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'SHADOW GATE / PIN '+name
    assert str((native/'gdb-out.log').resolve().relative_to(ROOT)) in recorded, 'SHADOW GATE / PINNED NATIVE'
    a, b = (original/'session.log').read_text(), (native/'gdb-out.log').read_text()
    assert a.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'SHADOW GATE / ORIGINAL COMPLETION'
    for sequence in range(28):
        compare(original, canonical, sequence)
    assert 'INTEGRATION AUDIT code=1 input=1\n' in b and 'ROUTE RESTORED\n' in b, 'SHADOW GATE / NATIVE AUDIT'
    assert b.count('CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n') == 1, 'SHADOW GATE / RETAIN FAILURE'
    restored(native, b, 'a1200-030')
    original_gate, native_gate = gate(a), gate(b)
    assert original_gate == dict(wait_start=47, sound_complete=149, shadow_creation=150), 'SHADOW GATE / ORIGINAL PHASE'
    assert native_gate == dict(wait_start=47, sound_complete=148, shadow_creation=149), 'SHADOW GATE / NATIVE PHASE'
    receipt = json.loads((native/'build-receipt.json').read_text())
    baseline = json.loads((ROOT/'amiga/.run-jFpn9K/build-receipt.json').read_text())
    assert receipt == baseline, 'SHADOW GATE / FLAME OBSERVER BUILD'
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        assert hashlib.sha256((native/name).read_bytes()).hexdigest() == receipt[key], 'SHADOW GATE / EXECUTABLE IDENTITY'
    for name, digest in pins(ROOT/'manifest/gameplay-item16-flame-events-native.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'SHADOW GATE / BASELINE PIN '+name
    assert len(re.findall(r'^ROUTE CHECKPOINT ordinal=', b, re.M)) == 36, 'SHADOW GATE / CAPTURE COUNT'
    for label in [f'route-{i}' for i in range(36)]+['route-rejected-entry']:
        actual = (native/f'{label}-a5.bin').read_bytes()
        before = (ROOT/'amiga/.run-jFpn9K'/f'{label}-a5.bin').read_bytes()
        for distance, size in ((0x5208, 372), (0x18fc, 4), (0x349a, 2)):
            assert actual[-distance:-distance+size] == before[-distance:-distance+size], 'SHADOW GATE / BASELINE GAME STATE'
        for suffix in ('screen', 'context'):
            assert (native/f'{label}-{suffix}.bin').read_bytes() == (ROOT/'amiga/.run-jFpn9K'/f'{label}-{suffix}.bin').read_bytes(), 'SHADOW GATE / BASELINE FRAME'
    for changed in (a.replace('SHADOW_GATE segment=10', 'MISSING segment=10'),
                    a.replace('site=3C76 step=0095 result=0000', 'site=3C76 step=0095 result=0001')):
        assert changed != a, 'SHADOW GATE / EFFECTIVE CONTROL'
        try:
            gate(changed)
        except AssertionError:
            pass
        else:
            raise AssertionError('SHADOW GATE / CORRUPTION ACCEPTED')
    print(json.dumps(dict(original=original_gate, native=native_gate,
                         shadow_lead=original_gate['shadow_creation']-native_gate['shadow_creation'],
                         original_repeat_matches=True, native_baseline_captures_match=37, corruption_controls=2,
                         whole_route_accepted=False), indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    check(parser.parse_args().native)
