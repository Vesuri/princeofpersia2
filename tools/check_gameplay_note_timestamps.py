"""Compare queued-note times separately from delivery; retain strict route failures."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re

from extract_original_data import ROOT, pins
from check_gameplay_music_window import window
from check_gameplay_exit_gate import gate
from inspect_gameplay_rejected_entry import inspect


def notes(log, original, kind=9):
    assert kind in (9, 12), 'NOTE TIME / REQUEST KIND'
    blocks, rows = [], []
    clock = None
    selector = None
    due = None
    for line in log.splitlines():
        if original:
            returned = re.match(r'AUDIO_RETURN selector=F d0=(\w+)', line)
            if returned:
                clock = int(returned[1], 16)
            call = re.match(r'AUDIO_CALL selector=(\w+).* packet=([0-9A-F/]+)', line)
            if not call:
                continue
            selector = int(call[1], 16)
            packet = bytes.fromhex(call[2].replace('/', ''))
            if selector == 6:
                blocks.append(rows)
                rows = []
            if selector not in (9, 12):
                continue
            assert len(packet) == 16 and packet[14] == 1, 'NOTE TIME / ORIGINAL ACTIVE RECORD'
            due = int.from_bytes(packet[10:14], 'big')
            packet = packet[:10]
        else:
            call = re.match(r'ROUTE_AUDIO step=\d+ selector=(\d+) result=(\w+) last_clock=(\w+)', line)
            if call:
                assert due is None, 'NOTE TIME / UNCONSUMED TIMESTAMP'
                selector = int(call[1])
                clock = int(call[3], 16)
                if selector == 6:
                    blocks.append(rows)
                    rows = []
                continue
            timestamp = re.fullmatch(r'ROUTE_NOTE_TIME selector=(\d+) due=(\w+) origin=(\w+)', line)
            if timestamp:
                assert selector in (9, 12) and int(timestamp[1]) == selector and due is None, 'NOTE TIME / TIMESTAMP OWNER'
                due = int(timestamp[2], 16)
                continue
            packet_match = re.fullmatch(r'ROUTE_AUDIO_PACKET words=([0-9A-F/]+)', line)
            if not packet_match:
                continue
            if selector not in (9, 12):
                due = None
                continue
            packet = bytes.fromhex(packet_match[1].replace('/', ''))
        assert clock is not None and due is not None and len(packet) == 10, 'NOTE TIME / COMPLETE NOTE'
        assert 0 <= clock-due <= 8, 'NOTE TIME / DELIVERY DEADLINE'
        rows.append((selector, due, clock, packet))
        due = None
    assert due is None, 'NOTE TIME / INCOMPLETE FINAL NOTE'
    tracks = [rows for rows in blocks if sum(s == 9 for s, *_ in rows) == 194]
    assert len(tracks) == 1, 'NOTE TIME / COMPLETE TRACK'
    # Reuse the independent request/return parser to establish track identity.
    selected = [(due, clock, packet) for selector, due, clock, packet in tracks[0] if selector == kind]
    assert len(selected) == 194, 'NOTE TIME / NOTE COVERAGE'
    if kind == 9:
        _, _, observed = window(log, original)
        assert [(clock, packet) for _, clock, packet in selected] == observed, 'NOTE TIME / REQUEST PAIR'
    return selected


def report(original, native):
    assert Counter(p for _, _, p in original) == Counter(p for _, _, p in native), 'NOTE TIME / SAME REQUESTS'
    def keyed(rows):
        counts = Counter()
        result = {}
        start = min(d for d, _, _ in rows)
        for due, clock, packet in rows:
            key = (packet, counts[packet])
            counts[packet] += 1
            result[key] = due-start
        return result
    a, b = keyed(original), keyed(native)
    differences = Counter(b[key]-a[key] for key in a)
    return dict(notes=194,
                original_enqueue_span=max(d for d, _, _ in original)-min(d for d, _, _ in original),
                native_enqueue_span=max(d for d, _, _ in native)-min(d for d, _, _ in native),
                original_delivery_lateness=dict(sorted(Counter(c-d for d, c, _ in original).items())),
                native_delivery_lateness=dict(sorted(Counter(c-d for d, c, _ in native).items())),
                relative_enqueue_differences=dict(sorted(differences.items())),
                pairing='same packet and occurrence, relative to earliest queued note',
                whole_route_accepted=False)


def check(native):
    for manifest in ('gameplay-item16-exit-audio.sha256', 'gameplay-item16-note-time-native.sha256'):
        recorded = pins(ROOT/'manifest'/manifest)
        for name, digest in recorded.items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'NOTE TIME / PIN '+name
    assert str((native/'gdb-out.log').resolve().relative_to(ROOT)) in recorded, 'NOTE TIME / PINNED RUN'
    a = (ROOT/'ref/mame/runs/item16-exit-level6-audio-1/session.log').read_text()
    b = (native/'gdb-out.log').read_text()
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in a, 'NOTE TIME / ORIGINAL COMPLETION'
    assert 'ROUTE RESTORED\n' in b and 'INTEGRATION AUDIT code=1 input=1\n' in b, 'NOTE TIME / NATIVE AUDIT'
    endpoint = inspect(native, ROOT/'ref/mame/runs/gameplay-level6-door-1', ROOT/'manifest/gameplay-assisted-level6-door.sha256')
    assert gate(a, True) == (122, 333) and gate(b, False) == (122, 331), 'NOTE TIME / SAME EXIT GATE'
    result = dict(endpoint, **report(notes(a, True), notes(b, False)))
    result['note_offs'] = report(notes(a, True, 12), notes(b, False, 12))
    result['clock_windows'] = []
    for log, original in ((a, True), (b, False)):
        first, last, _ = window(log, original)
        final = notes(log, original, 12)[-1]
        result['clock_windows'].append(dict(original=original, query_span=last-first,
                                           final_note_off_due=final[0]-first,
                                           final_note_off_delivery=final[1]-first,
                                           queries_after_final_note_off=last-final[1]))
    for changed in (b.replace('ROUTE_NOTE_TIME selector=9', 'MISSING selector=9', 1),
                    re.sub(r'(ROUTE_NOTE_TIME selector=9 due=)\w+', r'\g<1>FFFFFFFF', b, count=1)):
        try:
            notes(changed, False)
        except AssertionError:
            pass
        else:
            raise AssertionError('NOTE TIME / CORRUPTION ACCEPTED')
    result['corruption_controls'] = 2
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    check(parser.parse_args().native)
