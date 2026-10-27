"""Original System 7.5.5 master-block and NewPtr placement paired by test_mac_heap.cpp.

Each case uses every free master, then allocates one nonrelocatable block. The
new block must sit at the lowest position above pinned blocks, an implicit
handle's data must follow it directly, and the movable block in the way must
move, even across locked handles. Relocation targets follow the original's
rover and are recorded, not paired. A relocatable request that fits only by
moving a movable handle across a locked one succeeds; a larger one fails.
"""
import argparse
import re
from pathlib import Path

CASES = (('unlocked-implicit', 4, ''), ('hole-implicit', 4, ''), ('hole-explicit', 4, ''),
         ('hole-pointer', 4, ''), ('locked-low-implicit', 5, 'L1'), ('locked-run-implicit', 3, ''),
         ('locked-full-implicit', 4, ''), ('locked-full-pointer', 4, ''))
RELOCATE = re.compile(r'^MASTERS relocate request=(\d+) error=(-?\d+) free=(\d+) h1=0x([0-9A-F]+)->0x([0-9A-F]+) l2=0x([0-9A-F]+) l3=0x([0-9A-F]+) new=0x([0-9A-F]+)$', re.M)
ROW = re.compile(r'^MASTERS (\S+) spare=(\d+) order=(\S+) moved=(\S+) at=(\S+)$', re.M)


def check(text):
    assert 'FAIL REFERENCE' not in text and text.count('MASTERS_FIXTURE_COMPLETE cases=8\n') == 1, 'MASTERS / COMPLETION'
    rows = ROW.findall(text)
    assert [r[0] for r in rows] == [c[0] for c in CASES], 'MASTERS / CASES'
    for (name, spare, order, moved, at), (_, masters, below) in zip(rows, CASES):
        assert int(spare) == masters, 'MASTERS / SPARE ' + name
        order = order.split(',')
        new = 'P' if name.endswith('pointer') else 'M'
        index = order.index(new)
        assert ''.join(order[:index]) == below, 'MASTERS / BELOW ' + name
        offsets = [int(item.split('@0x')[1], 16) for item in at.split(',')]
        assert offsets == sorted(offsets), 'MASTERS / ORDER ' + name
        if name.endswith('implicit'):
            assert order[index + 1] == 'd', 'MASTERS / DATA ' + name
        if below:
            assert moved == '-', 'MASTERS / PINNED ' + name
        else:
            assert moved == 'h1', 'MASTERS / MOVED ' + name
            assert order.index('h1') > index, 'MASTERS / MOVED ABOVE ' + name
    moves = RELOCATE.findall(text)
    assert [(m[0], m[1]) for m in moves] == [('11264', '0'), ('13312', '-108')], 'MASTERS / RELOCATE RESULTS'
    fit, full = [[int(v, 16) for v in m[3:]] for m in moves]
    assert fit[0] == fit[4] < fit[2] < fit[1] < fit[3], 'MASTERS / RELOCATE ACROSS LOCKED'
    assert full[0] == full[1] and full[4] == 0, 'MASTERS / RELOCATE FAILURE UNCHANGED'
    return len(rows) + len(moves)


def controls(text):
    rejected = 0
    for old, new in [('MASTERS_FIXTURE_COMPLETE', 'INCOMPLETE'),
                     ('unlocked-implicit spare=4', 'unlocked-implicit spare=5'),
                     ('order=M,d,h2', 'order=h2,M,d'), ('order=L1,M,d', 'order=M,L1,d'),
                     ('order=P,L2', 'order=L2,P'), ('locked-full-implicit spare=4 order=M,d', 'locked-full-implicit spare=4 order=d,M'),
                     ('moved=-', 'moved=h2'), ('request=11264 error=0', 'request=11264 error=-108'),
                     ('request=13312 error=-108', 'request=13312 error=0'), ('h1=0x64->0x347C', 'h1=0x64->0x64')]:
        changed = text.replace(old, new, 1)
        assert changed != text, old
        try:
            check(changed)
        except AssertionError:
            rejected += 1
        else:
            raise AssertionError('MASTERS / ACCEPTED ALTERED EVIDENCE ' + old)
    return rejected


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference', type=Path)
    a = p.parse_args()
    text = (a.reference / 'session.log').read_text()
    cases = check(text)
    print('MASTERS ORIGINAL PASS: %d placements; %d altered records rejected' % (cases, controls(text)))
