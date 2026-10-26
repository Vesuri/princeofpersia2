"""Check the idle second-attract credits path against the original (item 23).

Credits, attract and capture events compare strictly. The attract demo has
two measured timing consequences accepted under D18, each reported with its
cause: the PlayAmbient draw (CODE 5 return +50BA) occurs at a different step
with the same value, and the death-sound gate (CODE 4+3C60) completes earlier,
starting the A5-4FCA countdown earlier. Nothing else may differ.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

from check_gameplay_opening import compare_capture
from extract_original_data import ROOT, pins

RELEASE = {'-DPOP2_' + flag for flag in ('CORE', 'DISPLAY', 'INPUT', 'UI', 'AUDIO')}
EVENTS = re.compile(r'^(?:CREDITS (?:ENTER|PENSTATE|PAGE|RETURN|CYCLE COMPLETE)|ATTRACT (?:NIS|RETURN)).*$', re.M)
FRAME = re.compile(r'^DEMO FRAME step=(\d+) seed=([0-9a-f]{8}) level=(\d+) room=(\d+) clock=(\d+)$', re.M)
RANDOM = re.compile(r'^DEMO RANDOM step=(\d+) site=([0-9a-f]{4}) result=([0-9a-f]{4}) seed=([0-9a-f]{8}) caller=([0-9a-f]{8})$', re.M)
SOUND = re.compile(r'^DEMO SOUNDON step=(\d+) site=(3c6a|3c78) result=([0-9a-f]{4})$', re.M)
LABELS = ['credits-page-1', 'credits-page-2', 'credits-page-3', 'credits-page-4', 'credits-after']
# A5-relative state: attract level, remaining minutes and death countdown
# state, startup skip flags, step, level, room and the QuickDraw random seed.
STATE = ((0x4faa, 2), (0x4fce, 2), (0x4fcc, 2), (0x524c, 2), (0x4ffc, 2),
         (0x349a, 2), (0x4fd2, 2), (0x4fc2, 2), (0x18fc, 4))
COUNTDOWN = 0x4fca
AMBIENT_RETURN = 0x50ba


def ambient(draws, base):
    assert len(draws) == 1, 'CREDITS / AMBIENT DRAW COUNT'
    step, site, result, seed, caller = draws[0]
    assert site == '6e1e' and int(caller, 16) - base == AMBIENT_RETURN, 'CREDITS / AMBIENT CALLER'
    return int(step), result, seed


def completion(sound, site):
    rows = [(int(step), result) for step, where, result in sound if where == site]
    assert rows and all(b[0] == a[0] + 1 for a, b in zip(rows, rows[1:])), 'CREDITS / SOUND CONTIGUITY'
    done = [step for step, result in rows if result == '0000']
    return rows[0][0], done[0] if done else None, rows


def demo(native_log, original_log):
    native = [tuple(row) for row in FRAME.findall(native_log)]
    original = [tuple(row) for row in FRAME.findall(original_log)]
    assert len(original) >= 500 and len(native) == len(original), 'CREDITS / DEMO FRAMES'
    assert [r[0] for r in native] == [r[0] for r in original], 'CREDITS / DEMO STEPS'
    assert [r[2:4] for r in native] == [r[2:4] for r in original], 'CREDITS / DEMO ROOMS'
    native_base = int(re.search(r'^CREDITS SEGMENTS 4=[0-9a-f]{8} 5=([0-9a-f]{8})', native_log, re.M).group(1), 16)
    original_base = int(re.search(r'^MAP seg=5 base=([0-9A-F]{8})', original_log, re.M).group(1), 16)
    a, value, seed = ambient(RANDOM.findall(original_log), original_base)
    b, native_value, native_seed = ambient(RANDOM.findall(native_log), native_base)
    assert (native_value, native_seed) == (value, seed) and 0 < abs(b - a) <= 4, 'CREDITS / AMBIENT VALUE'
    first, last = min(a, b), max(a, b)
    seeds = {int(r[0]): r[1] for r in original}
    for n, o in zip(native, original):
        step = int(n[0])
        if first <= step < last:
            # The later draw leaves that side at the previous step's seed.
            lagging = native if b > a else original
            leading = original if b > a else native
            assert dict((int(r[0]), r[1]) for r in lagging)[step] == seeds[first - 1], 'CREDITS / AMBIENT LAG'
            assert dict((int(r[0]), r[1]) for r in leading)[step] == seed, 'CREDITS / AMBIENT LEAD'
        else:
            assert n[1] == o[1], 'CREDITS / DEMO SEED %d' % step
    native_sound, original_sound = SOUND.findall(native_log), SOUND.findall(original_log)
    start, done, rows = completion(original_sound, '3c78')
    native_start, native_done, native_rows = completion(native_sound, '3c78')
    first_start, _, first_rows = completion(original_sound, '3c6a')
    native_first_start, _, native_first_rows = completion(native_sound, '3c6a')
    assert done and native_done and start == native_start == first_start == native_first_start, 'CREDITS / SOUND WAIT'
    lead = done - native_done
    assert 0 < lead <= 2, 'CREDITS / SOUND LEAD'
    assert rows[:len(native_rows) - 1] == native_rows[:-1] and all(r == '0000' for _, r in first_rows + native_first_rows), 'CREDITS / SOUND RESULTS'
    clocks = {int(r[0]): int(r[4]) for r in original}
    for n in native:
        step, clock = int(n[0]), int(n[4])
        if step <= native_done:
            assert clock == 0 and clocks[step] == 0, 'CREDITS / COUNTDOWN BEFORE DEATH'
        elif step + lead in clocks:
            assert clock == clocks[step + lead] and clock > 0, 'CREDITS / COUNTDOWN LEAD'
    return len(native), (a, b), (done, native_done), lead


def check(native, reference):
    for name, digest in pins(ROOT / 'manifest' / 'credits-reference.sha256').items():
        path = reference / Path(name).name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'CREDITS / REFERENCE PIN'
    receipt = json.loads((native / 'build-receipt.json').read_text())
    assert set(receipt['flags'].split()) == RELEASE, 'CREDITS / RELEASE BUILD'
    log = (native / 'gdb-out.log').read_text()
    original = (reference / 'session.log').read_text()
    assert original.count('CREDITS_REFERENCE_COMPLETE\n') == 1 and 'FAIL REFERENCE' not in original, 'CREDITS / ORIGINAL COMPLETION'
    assert 'CORE STOP' not in log and 'UI OVERLAY' not in log and 'INVALID' not in log, 'CREDITS / NATIVE STOP'
    actual, expected = EVENTS.findall(log), EVENTS.findall(original)
    for marker in ('CREDITS ENTER level=0 minutes=75', 'CREDITS RETURN result=0 pages=4 pens=14',
                   'CREDITS CYCLE COMPLETE level=4 room=1'):
        assert expected.count(marker) == 1, 'CREDITS / ORIGINAL ' + marker
    assert sum(line.startswith('CREDITS PENSTATE') for line in expected) == 14, 'CREDITS / ORIGINAL PEN STATES'
    assert actual == expected, 'CREDITS / EVENT SEQUENCE'
    frames, draws, completions, lead = demo(log, original)
    transfer = (ROOT / 'build/display/video-transfer.bin').read_bytes()
    for label in LABELS:
        args = [(native / (label + '-screen.bin')).read_bytes(),
                (native / (label + '-colors.bin')).read_bytes(), transfer,
                (reference / (label + '.indices')).read_bytes(),
                (reference / (label + '.rgb')).read_bytes()]
        compare_capture(*args)
        a5, original_a5 = ((path / (label + '-a5.bin')).read_bytes() for path in (native, reference))
        assert len(a5) == len(original_a5) == 22266, 'CREDITS / A5 EXTENT'
        for distance, size in STATE:
            assert a5[-distance:-distance+size] == original_a5[-distance:-distance+size], 'CREDITS / STATE %s %04X' % (label, distance)
        countdown = [int.from_bytes(data[-COUNTDOWN:-COUNTDOWN+2], 'big') for data in (a5, original_a5)]
        assert countdown[0] + lead == countdown[1] or countdown[0] == countdown[1] == 0, 'CREDITS / COUNTDOWN ' + label
        for field, offset in ((0, 48 * 640 + 65), (4, 0)):
            changed = list(args)
            data = bytearray(changed[field])
            data[offset] ^= 1
            changed[field] = bytes(data)
            try:
                compare_capture(*changed)
            except AssertionError:
                pass
            else:
                raise AssertionError('CREDITS / CORRUPTION CONTROL ACCEPTED')
    controls = 0
    for old, new in (('CREDITS RETURN result=0', 'CREDITS RETURN result=1'), ('result=aeab', 'result=aeac'),
                     ('site=3c78 result=0000', 'site=3c78 result=0001')):
        try:
            if old.startswith('CREDITS'):
                assert EVENTS.findall(log.replace(old, new, 1)) == expected, 'CREDITS / EVENT SEQUENCE'
            else:
                demo(log.replace(old, new, 1), original)
        except (AssertionError, IndexError, KeyError, TypeError):
            controls += 1
        else:
            raise AssertionError('CREDITS / EVIDENCE CONTROL ACCEPTED ' + old)
    print('CREDITS PASS:', len(actual), 'strict credits/attract events,', frames, 'demo frames;',
          len(LABELS), 'paired pixel/colour/state captures;', len(LABELS) * 2 + controls, 'corruption controls')
    print('CREDITS D18: PlayAmbient draw at original step %d, native %d (same value);' % draws,
          'death-sound gate completes at original step %d, native %d (countdown lead %d)' % (*completions, lead))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.native, args.reference)
