"""Qualify read-only flame reset events without accepting the native route."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from extract_original_data import ROOT, pins
from replay_gameplay_route import compare
from check_core_result import restored


def events(log):
    rows = [tuple(int(v, 16 if i != 1 else 10) for i, v in enumerate(row))
            for row in re.findall(r'^FLAME_EVENT site=(\w+) step=(\d+) frame=(\w+) player=(\w+)$', log, re.M)]
    updates = [row for row in rows if row[0] == 0x2cc2]
    resets = [row for row in rows if row[0] != 0x2cc2]
    assert [row[1] for row in updates] == list(range(160, 241)), 'FLAME EVENTS / COMPLETE WINDOW'
    assert len(resets) == 1 and resets[0][0] == 0x29f8 and resets[0][2] == 154, 'FLAME EVENTS / CHARACTER RESET'
    return updates, resets


def captures(run, log):
    updates, resets = events(log)
    for site, step, frame, player in updates+resets:
        prefix = run/f'flame-event-{site:04X}-{step}'
        a = Path(str(prefix)+'-a5.bin').read_bytes()
        c = Path(str(prefix)+'-context.bin').read_bytes()
        assert len(a) == 22266 and len(c) == 0x2200, 'FLAME EVENTS / CAPTURE EXTENT'
        word = lambda distance: int.from_bytes(a[-distance:-distance+2], 'big')
        assert (word(0x349a), word(0x504c), word(0x50c8)) == (step, frame, player), 'FLAME EVENTS / LOG PAIR'
    reset_step = resets[0][1]
    following = (run/f'flame-event-2CC2-{reset_step+1}-context.bin').read_bytes()
    assert int.from_bytes(following[0x914:0x916], 'big') == 133, 'FLAME EVENTS / RESET PUBLICATION'
    return reset_step


def verify_pins(side):
    recorded = pins(ROOT/f'manifest/gameplay-item16-flame-events-{side}.sha256')
    for name, digest in recorded.items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'FLAME EVENTS / PIN '+name
    return recorded


def check(native=None):
    verify_pins('original')
    original = ROOT/'ref/mame/runs/item16-flame-events-1'
    canonical = ROOT/'ref/mame/runs/gameplay-assisted-level13-flame-1'
    a = (original/'session.log').read_text()
    assert a.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'FLAME EVENTS / ORIGINAL COMPLETION'
    for sequence in range(28):
        compare(original, canonical, sequence)
    original_step = captures(original, a)
    assert original_step == 228, 'FLAME EVENTS / ORIGINAL RESET STEP'
    for changed in (a.replace('FLAME_EVENT site=2CC2', 'MISSING site=2CC2', 1),
                    a.replace('FLAME_EVENT site=29F8 step=228 frame=009A', 'FLAME_EVENT site=29F8 step=228 frame=009B')):
        try:
            events(changed)
        except AssertionError:
            continue
        raise AssertionError('FLAME EVENTS / CORRUPTION ACCEPTED')
    report = dict(original_reset_step=original_step, observed_updates=81,
                  original_repeat_matches=True, corruption_controls=2, whole_route_accepted=False)
    if native:
        recorded = verify_pins('native')
        assert str((native/'gdb-out.log').resolve().relative_to(ROOT)) in recorded, 'FLAME EVENTS / UNPINNED NATIVE'
        b = (native/'gdb-out.log').read_text()
        assert 'ROUTE RESTORED\n' in b and 'INTEGRATION AUDIT code=1 input=1\n' in b, 'FLAME EVENTS / NATIVE AUDIT'
        restored(native, b, 'a1200-030')
        report['native_reset_step'] = captures(native, b)
        actor_differences = []
        for step in range(160, 241):
            original_a5 = (original/f'flame-event-2CC2-{step}-a5.bin').read_bytes()
            native_a5 = (native/f'flame-event-2CC2-{step}-a5.bin').read_bytes()
            slots = [slot for slot in range(6)
                     if original_a5[-0x5208+slot*62:-0x5208+(slot+1)*62] != native_a5[-0x5208+slot*62:-0x5208+(slot+1)*62]]
            if slots:
                actor_differences.append(dict(step=step, slots=slots))
        assert report['native_reset_step'] == 227, 'FLAME EVENTS / NATIVE RESET STEP'
        assert b.count('CORE REASON GAMEPLAY ROUTE / FIRST FRAME\n') == 1, 'FLAME EVENTS / RETAIN STRICT FAILURE'
        assert actor_differences == [dict(step=step, slots=[1]) for step in range(160, 241)], 'FLAME EVENTS / SHADOW ONLY'
        for step in range(160, 240):
            original_a5 = (original/f'flame-event-2CC2-{step+1}-a5.bin').read_bytes()
            native_a5 = (native/f'flame-event-2CC2-{step}-a5.bin').read_bytes()
            assert original_a5[-0x5208:-0x5208+372] == native_a5[-0x5208:-0x5208+372], 'FLAME EVENTS / ONE FRAME SHIFT'
        report['shifted_actor_pairs'] = 80
        report['actor_differences'] = actor_differences
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path)
    args = parser.parse_args()
    check(args.native)
