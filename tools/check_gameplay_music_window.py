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



def clock_batches(log, original, first, last):
    pattern = (r'^AUDIO_RETURN selector=F d0=(\w+)' if original else
               r'^ROUTE_AUDIO step=\d+ selector=15 result=(\w+)')
    queries = [int(value, 16) for value in re.findall(pattern, log, re.M)]
    assert queries and all(b >= a for a, b in zip(queries, queries[1:])), 'MUSIC CLOCK / MONOTONIC QUERIES'
    selected = [value for value in queries if first <= value <= last]
    assert selected and selected[0] == first and selected[-1] == last, 'MUSIC CLOCK / BOUNDED WINDOW'
    increments = Counter(b-a for a, b in zip(selected, selected[1:]))
    return dict(queries=len(selected), increments=dict(sorted(increments.items())))


def original_clock_phase(log, a5):
    assert len(a5) == 22266, 'MUSIC CLOCK / A5 EXTENT'
    origin = int.from_bytes(a5[-0x11d4:-0x11d0], 'big')
    blocks, rows = [], []
    for line in log.splitlines():
        match = re.match(r'AUDIO_CALL selector=(\w+).* tick=(\w+) clock=(\w+)', line)
        if not match:
            continue
        selector, tick, clock = (int(value, 16) for value in match.groups())
        if selector == 6:
            blocks.append(rows)
            rows = []
        rows.append((selector, tick, clock))
    selected = [rows for rows in blocks if sum(s == 9 for s, _, _ in rows) == 194]
    assert len(selected) == 1, 'MUSIC CLOCK / PHASE TRACK'
    phase = Counter(tick-origin-clock for selector, tick, clock in selected[0] if selector == 9)
    assert phase == {-1: 28, 0: 93, 1: 69, 2: 4}, 'MUSIC CLOCK / ORIGINAL NOTE PHASE'
    return dict(origin=origin, note_call_tick_minus_origin_minus_driver_clock=dict(sorted(phase.items())))


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
    original_batches = clock_batches(a, True, first, last)
    native_batches = clock_batches(b, False, nfirst, nlast)
    original_a5 = (original/'route-12-a5.bin').read_bytes()
    phase = original_clock_phase(a, original_a5)
    changed_a5 = bytearray(original_a5)
    changed_a5[-0x11d4] ^= 1
    try:
        original_clock_phase(a, changed_a5)
    except AssertionError:
        pass
    else:
        raise AssertionError('MUSIC CLOCK / ORIGIN CORRUPTION ACCEPTED')
    assert original_batches == dict(queries=1129, increments={0: 722, 2: 94, 3: 312}), 'MUSIC CLOCK / ORIGINAL BATCHES'
    assert native_batches == dict(queries=1116, increments={0: 185, 1: 744, 2: 186}), 'MUSIC CLOCK / NATIVE BATCHES'
    for text, original, start, end, pattern in (
            (a, True, first, last, r'(^AUDIO_RETURN selector=F d0=)\w+'),
            (b, False, nfirst, nlast, r'(^ROUTE_AUDIO step=\d+ selector=15 result=)\w+')):
        corrupt = re.sub(pattern, r'\g<1>FFFFFFFF', text, count=1, flags=re.M)
        try:
            clock_batches(corrupt, original, start, end)
        except AssertionError:
            continue
        raise AssertionError('MUSIC CLOCK / BACKWARDS CONTROL ACCEPTED')
    report = dict(endpoint, note_requests=194, note_multiset_matches=True,
                  note_order_mismatch_indices=[i for i, (x, y) in enumerate(zip(notes, nnotes)) if x[1] != y[1]],
                  original_clock_query_span=last-first, native_clock_query_span=nlast-nfirst,
                  clock_scope='most recent selector-15 return at each request; not an exact event timestamp',
                  underlying_timing_resolved=False, missing_evidence_controls=2,
                  original_clock_batches=original_batches, native_clock_batches=native_batches,
                  original_clock_phase=phase, clock_corruption_controls=2,
                  origin_corruption_controls=1)
    (native/'music-window-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
