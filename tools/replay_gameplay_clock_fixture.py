"""Replay a bounded music-clock counterfactual; never accept a complete route."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from extract_original_data import ROOT, pins
from gameplay_clock_fixture import verify
from gameplay_assisted_inputs import metadata
from gameplay_assist import assist
from replay_gameplay_route import compare, wait, status
from check_gameplay_music_window import window

REFERENCE = ROOT/'ref/mame/runs/gameplay-level6-door-1'
REFERENCE_PINS = ROOT/'manifest/gameplay-assisted-level6-door.sha256'


def application_log(log, data, sham=False):
    begin = re.findall(r'^CLOCK_FIXTURE_BEGIN step=(\w+) base=(\w+)$', log, re.M)
    assert len(begin) == 1 and int(begin[0][0], 16) == data['original_start_step'], 'CLOCK FIXTURE / BEGIN'
    base = int(begin[0][1], 16)
    lines, pending, count = [], None, 0
    for line in log.splitlines():
        returned = re.match(r'AUDIO_RETURN selector=F d0=(\w+)', line)
        if returned:
            pending = (len(lines), int(returned[1], 16))
        query = re.fullmatch(r'CLOCK_FIXTURE_QUERY index=(\w+) step=(\w+) before=(\w+) after=(\w+)', line)
        if query:
            count += 1
            assert int(query[1], 16) == count and count <= (2048 if sham else len(data['deltas'])), 'CLOCK FIXTURE / QUERY ORDER'
            assert pending and pending[1] == int(query[3], 16), 'CLOCK FIXTURE / RETURN OWNER'
            assert int(query[4], 16) == (int(query[3], 16) if sham else base+data['deltas'][count-1]), 'CLOCK FIXTURE / RESPONSE DERIVATION'
            index = pending[0]
            lines[index] = re.sub(r'd0=\w+', 'd0='+query[4], lines[index], count=1)
            pending = None
        lines.append(line)
    assert count > 0 and (sham or count == len(data['deltas'])), 'CLOCK FIXTURE / QUERY COUNT'
    ends = re.findall(r'^CLOCK_FIXTURE_TRACK_COMPLETE step=(\w+) queries=(\w+)$', log, re.M)
    assert len(ends) == 1 and int(ends[0][1], 16) == count, 'CLOCK FIXTURE / TRACK COMPLETION'
    assert log.count('CLOCK_FIXTURE_COMPLETE\n') == 1 and log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'CLOCK FIXTURE / POSITIVE FINISH'
    return '\n'.join(lines)+'\n', int(ends[0][0], 16)




def check_keys(log, data, sham):
    rows = re.findall(r'^CLOCK_FIXTURE_KEYS index=(\d+) frame=(\d+) step=(\d+) before=([0-9A-F]{32}) after=([0-9A-F]{32})$', log, re.M)
    reads = re.findall(r'^GAMEPLAY_ROUTE_KEYS frame=(\d+) step=(\d+) map=([0-9A-F]{32})$', log, re.M)
    assert rows and len(rows) == len(reads) and len(rows) <= len(data['keys']), 'CLOCK FIXTURE / KEY COVERAGE'
    for index, (row, read) in enumerate(zip(rows, reads), 1):
        expected = data['keys'][index-1]
        assert int(row[0]) == index and (int(row[1]), int(row[2]), row[4]) == (expected['frame'], expected['step'], expected['map']), 'CLOCK FIXTURE / PINNED KEY READ'
        assert read == (row[1], row[2], row[4]), 'CLOCK FIXTURE / KEY PUBLICATION'
    if sham:
        assert len(rows) == len(data['keys']), 'CLOCK FIXTURE / SHAM COMPLETE INPUT'
    return sum(row[3] != row[4] for row in rows)


def check_prefix_control():
    failed = ROOT/'ref/mame/runs/item16-music-clock-1'
    for name, digest in pins(ROOT/'manifest/gameplay-item16-clock-prefix-control.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'CLOCK FIXTURE / FAILED CONTROL PIN '+name
    log = (failed/'session.log').read_text()
    assert 'CLOCK_FIXTURE_STOP' in log and 'CLOCK_FIXTURE_BEGIN' not in log and 'CLOCK_FIXTURE_QUERY' not in log, 'CLOCK FIXTURE / PRE INTERVENTION CONTROL'
    canonical_log = (ROOT/'ref/mame/runs/item16-exit-level6-audio-1/session.log').read_text()
    key_pattern = r'^GAMEPLAY_ROUTE_KEYS (.*)$'
    a = re.findall(key_pattern, canonical_log, re.M)
    b = re.findall(key_pattern, log, re.M)
    assert [(i+1, x, y) for i, (x, y) in enumerate(zip(a, b)) if x != y] == [(2, 'frame=0 step=0 map=00000000000000000000000000000008', 'frame=0 step=0 map=00000000000000000000000000000000')], 'CLOCK FIXTURE / FAILED INPUT CONTROL'
    for sequence in range(7):
        compare(failed, REFERENCE, sequence)
    try:
        compare(failed, REFERENCE, 7)
    except ValueError as error:
        assert str(error) == 'ROUTE / REPEAT MISMATCH route-7-context.bin', 'CLOCK FIXTURE / FAILED CONTROL REASON'
    else:
        raise AssertionError('CLOCK FIXTURE / PREFIX CONTROL ACCEPTED')



def check_trigger_control():
    run = ROOT/'ref/mame/runs/item16-music-clock-control-2'
    for name, digest in pins(ROOT/'manifest/gameplay-item16-clock-trigger-control.sha256').items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'CLOCK FIXTURE / TRIGGER CONTROL PIN '+name
    log = (run/'session.log').read_text()
    assert 'CLOCK_FIXTURE_COMPLETE' in log and 'CLOCK_FIXTURE_BEGIN' not in log and 'CLOCK_FIXTURE_QUERY' not in log, 'CLOCK FIXTURE / UNREACHED TRIGGER CONTROL'
    for sequence in range(16):
        compare(run, REFERENCE, sequence)
    check_keys(log, verify(ROOT/'tmp/item16-music-clock-keys-fixture'), True)
    first, last, notes = window(log, True)
    baseline = (ROOT/'ref/mame/runs/item16-exit-level6-audio-1/session.log').read_text()
    assert (first, last, notes) == window(baseline, True), 'CLOCK FIXTURE / TRIGGER CONTROL SCHEDULE'


def check(run, fixture, sham=False):
    data = verify(fixture)
    check_prefix_control()
    check_trigger_control()
    metadata(REFERENCE, REFERENCE_PINS)
    for sequence in range(13):
        compare(run, REFERENCE, sequence)
    log = (run/'session.log').read_text()
    corrected_keys = check_keys(log, data, sham)
    observed, stop_step = application_log(log, data, sham)
    first, last, notes = window(observed, True)
    nfirst, nlast, native_notes = window((ROOT/'amiga/.run-eNDhoJ/gdb-out.log').read_text(), False)
    relative = lambda start, rows: [(clock-start, packet) for clock, packet in rows]
    baseline = (ROOT/'ref/mame/runs/item16-exit-level6-audio-1/session.log').read_text()
    bfirst, blast, baseline_notes = window(baseline, True)
    assert relative(bfirst, baseline_notes) != relative(nfirst, native_notes), 'CLOCK FIXTURE / UNMODIFIED CONTROL'
    if sham:
        for sequence in range(16):
            compare(run, REFERENCE, sequence)
        assert relative(first, notes) == relative(bfirst, baseline_notes) and last-first == blast-bfirst, 'CLOCK FIXTURE / SHAM SCHEDULE'
    else:
        assert relative(first, notes) == relative(nfirst, native_notes), 'CLOCK FIXTURE / NOTE SCHEDULE'
        assert last-first == nlast-nfirst, 'CLOCK FIXTURE / TRACK CLOCK SPAN'
    corrupt = re.sub(r'(^CLOCK_FIXTURE_QUERY index=1 .* after=)\w+', r'\g<1>FFFFFFFF', log, count=1, flags=re.M)
    try:
        application_log(corrupt, data, sham)
    except AssertionError:
        pass
    else:
        raise AssertionError('CLOCK FIXTURE / CORRUPTION ACCEPTED')
    report = dict(sham=sham, corrected_host_key_reads=corrected_keys, notes=len(notes), relative_note_schedule_matches=True,
                  track_clock_span=last-first, original_fixture_stop_step=stop_step,
                  baseline_matches_native=False, comparison='unmodified original' if sham else 'native', response_corruption_controls=1, whole_route_accepted=False)
    (run/'clock-fixture-comparison.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, sort_keys=True))


def replay(run, fixture, sham=False):
    verify(fixture);metadata(REFERENCE, REFERENCE_PINS)
    assert wait(run, 0, 900)['reason'] == 'first-frame', 'CLOCK FIXTURE / INITIAL'
    for line in (REFERENCE/'route-actions.txt').read_text().splitlines():
        fields = line.split();sequence = int(fields[0])
        assert sequence == int(status(run)['sequence'])+1, 'CLOCK FIXTURE / SEQUENCE'
        if fields[1] == 'assist':
            assist(run, fields[2], int(fields[3]))
        else:
            temporary = run/'route-command.tmp'
            temporary.write_text(line+'\n');temporary.replace(run/'route-command.txt')
            wait(run, sequence, 180)
        if sequence <= 12:
            compare(run, REFERENCE, sequence)
        print(f'CLOCK FIXTURE checkpoint={sequence} step={status(run)["step"]}', flush=True)
    check(run, fixture, sham)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('fixture', type=Path)
    parser.add_argument('--check-only', action='store_true')
    parser.add_argument('--sham', action='store_true')
    args = parser.parse_args()
    if args.check_only:
        recorded = pins(ROOT/('manifest/gameplay-item16-clock-control.sha256' if args.sham else 'manifest/gameplay-item16-clock-replay.sha256'))
        assert str((args.run/'session.log').resolve().relative_to(ROOT)) in recorded, 'CLOCK FIXTURE / UNPINNED RUN'
        for name, digest in recorded.items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, 'CLOCK FIXTURE / RUN PIN '+name
    (check if args.check_only else replay)(args.run, args.fixture, args.sham)
