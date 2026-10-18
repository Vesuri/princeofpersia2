"""Explain the retained level-six exit decision without accepting its failed route."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from extract_original_data import ROOT, pins
from gameplay_assisted_inputs import metadata
from inspect_gameplay_rejected_entry import inspect
from replay_gameplay_route import compare


def gate(log, original):
    lines = [line for line in log.splitlines() if line.startswith('ROUTE_EXIT ')]
    rows = []
    for line in lines:
        match = re.fullmatch(
            r'ROUTE_EXIT site=([0-9A-F]{4}) step=([0-9A-F]+) level=([0-9A-F]+) '
            r'next=([0-9A-F]+) cue=([0-9A-F]{4}) result=([0-9A-F]{4})'
            + (r' tick=([0-9A-F]{8})' if original else ''), line)
        assert match, 'EXIT GATE / MALFORMED OBSERVATION'
        values = match.groups()
        rows.append(tuple(int(v, 16 if original or i not in (1, 2, 3) else 10)
                          for i, v in enumerate(values[:6])))
    assert rows, 'EXIT GATE / NO OBSERVATIONS'
    start, end = rows[0][1], rows[-1][1]
    assert start < end, 'EXIT GATE / NO WAIT'
    expected = []
    for step in range(start, end+1):
        cue = 0x127 if step == start else 0x126
        expected.extend(((0x6134, step, 6, 7, cue, 0),
                         (0x6144, step, 6, 7, cue, int(step != end))))
    expected.extend(((0x6152, end, 6, 7, 0x126, 0),
                     (0x61ce, end, 6, 7, 0x126, 0)))
    assert rows == expected, 'EXIT GATE / BRANCH SEQUENCE'
    return start, end


def check(native, reference):
    recorded = pins(ROOT/'manifest/gameplay-item16-exit-gate.sha256')
    for path in (native/'gdb-out.log', reference/'session.log'):
        assert str(path.resolve().relative_to(ROOT)) in recorded, 'EXIT GATE / UNPINNED RUN'
    for name, digest in recorded.items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'EXIT GATE / PIN '+name
    canonical = ROOT/'ref/mame/runs/gameplay-level6-door-1'
    manifest = ROOT/'manifest/gameplay-assisted-level6-door.sha256'
    metadata(canonical, manifest)
    for sequence in range(len((canonical/'route-actions.txt').read_text().splitlines())+1):
        compare(reference, canonical, sequence)
    original_log = (reference/'session.log').read_text()
    assert original_log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'EXIT GATE / ORIGINAL INCOMPLETE'
    original_start, original_end = gate(original_log, True)
    native_log = (native/'gdb-out.log').read_text()
    native_start, native_end = gate(native_log, False)
    endpoint = inspect(native, canonical, manifest)
    assert original_start == native_start == 122, 'EXIT GATE / PENDING TRANSITION'
    assert original_end == endpoint['original_frames'] == 333, 'EXIT GATE / ORIGINAL END'
    assert native_end == endpoint['native_frames'] == 331, 'EXIT GATE / NATIVE END'
    # Lost, malformed and altered observations must not establish a cause.
    for changed in (native_log.replace('ROUTE_EXIT site=61CE', 'MISSING site=61CE'),
                    native_log.replace('step=200 ', 'step=201 '),
                    native_log.replace('site=6144 step=331 level=6 next=7 cue=0126 result=0000',
                                       'site=6144 step=331 level=6 next=7 cue=0126 result=0001')):
        try:
            gate(changed, False)
        except AssertionError:
            continue
        raise AssertionError('EXIT GATE / CORRUPTION CONTROL ACCEPTED')
    report = dict(endpoint, pending_transition_frame=122,
                  original_sound_gate_frame=original_end, native_sound_gate_frame=native_end,
                  underlying_audio_timing_resolved=False, corruption_controls=3)
    (native/'exit-gate-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.native, args.reference)
