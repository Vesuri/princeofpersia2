"""Compare observed music requests without accepting the rejected exit timing."""
import hashlib
import json
import re
from collections import Counter

from extract_original_data import ROOT, pins
from check_gameplay_exit_gate import gate
from inspect_gameplay_rejected_entry import inspect


def window(log, original):
    clock = None
    blocks = []
    notes = []
    start = None
    selector = None
    stack = []
    for line in log.splitlines():
        if original:
            m = re.match(r'AUDIO_CALL selector=(\w+).* sp=(\w+).* packet=([0-9A-F/]+)', line)
            if m:
                selector = int(m[1], 16)
                stack.append((selector, m[2]))
                packet = bytes.fromhex(m[3].replace('/', ''))[:10]
            else:
                m = re.match(r'AUDIO_RETURN selector=(\w+) d0=(\w+).* sp=(\w+)', line)
                if m:
                    assert stack and stack.pop() == (int(m[1], 16), m[3]), 'MUSIC WINDOW / RETURN PAIR'
                    if int(m[1], 16) == 15:
                        clock = int(m[2], 16)
                continue
        else:
            m = re.match(r'ROUTE_AUDIO step=(\d+) selector=(\d+) result=(\w+) last_clock=(\w+)', line)
            if m:
                selector = int(m[2])
                clock = int(m[4], 16)
                if selector == 9:
                    continue
            elif line.startswith('ROUTE_AUDIO_PACKET words='):
                assert selector in (9, 12, 17), 'MUSIC WINDOW / PACKET OWNER'
                packet = bytes.fromhex(line.split('=')[1].replace('/', ''))
                if selector != 9:
                    continue
            else:
                continue
        if selector == 6:
            if start is not None:
                blocks.append((start, clock, notes))
            start, notes = clock, []
        elif selector == 9:
            assert clock is not None and len(packet) == 10, 'MUSIC WINDOW / NOTE CLOCK'
            notes.append((clock, packet))
    assert not stack, 'MUSIC WINDOW / PENDING CALL'
    selected = [b for b in blocks if len(b[2]) == 194]
    assert len(selected) == 1, 'MUSIC WINDOW / COMPLETE TRACK'
    return selected[0]


def main():
    for name, digest in pins(ROOT/'manifest/gameplay-item16-exit-audio.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'MUSIC WINDOW / PIN '+name
    original = ROOT/'ref/mame/runs/item16-exit-level6-audio-1'
    native = ROOT/'amiga/.run-eNDhoJ'
    a = (original/'session.log').read_text()
    b = (native/'gdb-out.log').read_text()
    assert a.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'MUSIC WINDOW / ORIGINAL COMPLETION'
    assert gate(a, True) == (122, 333) and gate(b, False) == (122, 331), 'MUSIC WINDOW / EXIT'
    endpoint = inspect(native, ROOT/'ref/mame/runs/gameplay-level6-door-1',
                       ROOT/'manifest/gameplay-assisted-level6-door.sha256')
    first, last, notes = window(a, True)
    nfirst, nlast, nnotes = window(b, False)
    assert Counter(p for _, p in notes) == Counter(p for _, p in nnotes), 'MUSIC WINDOW / NOTE MULTISET'
    # Deleting a request must invalidate the complete-track observation.
    for log, is_original, marker in ((a, True, 'AUDIO_CALL selector=9 '),
                                      (b, False, 'ROUTE_AUDIO_PACKET words=')):
        changed = log.replace(marker, 'REMOVED ', 1) if is_original else log.replace(marker, 'REMOVED ')
        try:
            window(changed, is_original)
        except AssertionError:
            continue
        raise AssertionError('MUSIC WINDOW / MISSING-EVIDENCE CONTROL ACCEPTED')
    report = dict(endpoint, note_requests=194, note_multiset_matches=True,
                  note_order_mismatch_indices=[i for i, (x, y) in enumerate(zip(notes, nnotes)) if x[1] != y[1]],
                  original_clock_query_span=last-first, native_clock_query_span=nlast-nfirst,
                  clock_scope='most recent selector-15 return at each request; not an exact event timestamp',
                  underlying_timing_resolved=False, missing_evidence_controls=2)
    (native/'music-window-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
