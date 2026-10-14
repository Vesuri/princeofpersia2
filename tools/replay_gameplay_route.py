"""Replay authored routes through levels one through four in a fresh original run."""
import argparse
import hashlib
from pathlib import Path
import time

from extract_original_data import ROOT, pins

KEYS = {'left', 'right', 'up', 'down', 'shift', 'control', 'space',
        'enter', 'escape', 'command', 'r'}


def inputs(path):
    rows = []
    for line in path.read_text().splitlines():
        if not line.strip() or line.startswith('#'):
            continue
        count, keys = line.split()
        count = int(count)
        if not 1 <= count <= 600 or not (keys == 'none' or
                set(keys.split(',')) <= KEYS):
            raise ValueError('ROUTE / INPUT FORMAT')
        rows.append((count, keys))
    if not rows:
        raise ValueError('ROUTE / EMPTY INPUTS')
    return rows


def status(run):
    try:
        return dict(field.split('=') for field in
                    (run / 'route-status.txt').read_text().split())
    except FileNotFoundError:
        return {}


def wait(run, sequence, seconds):
    deadline = time.monotonic() + seconds
    while True:
        current = status(run)
        actual = int(current.get('sequence', -1))
        if actual == sequence:
            return current
        if actual > sequence:
            raise ValueError('ROUTE / RUN ALREADY ADVANCED')
        if time.monotonic() >= deadline:
            raise TimeoutError('ROUTE / OBSERVATION DEADLINE; inspect the existing '
                               'emulator before taking further action')
        time.sleep(.2)


def compare(run, reference, sequence, *, reference_sequence=None):
    # Full context and pixels are repeatable here. Full A5 includes platform
    # timers; compare actor records, RNG and route counters separately.
    reference_sequence = sequence if reference_sequence is None else reference_sequence
    for suffix, size in (('-context.bin', 0x2200), ('.indices', 640 * 480),
                         ('.rgb', 768), ('-a5.bin', 22266)):
        name = f'route-{sequence}{suffix}'
        actual = (run / name).read_bytes()
        expected = (reference / f'route-{reference_sequence}{suffix}').read_bytes()
        if len(actual) != size or len(expected) != size:
            raise ValueError('ROUTE / CAPTURE EXTENT ' + name)
        if suffix == '-a5.bin':
            spans = ((-0x5208, 6 * 62), (-0x18fc, 4), (-0x4fd2, 2),
                     (-0x4fc2, 2), (-0x349a, 2))
            actual = b''.join(actual[start:start + size] for start, size in spans)
            expected = b''.join(expected[start:start + size] for start, size in spans)
        if actual != expected:
            raise ValueError('ROUTE / REPEAT MISMATCH ' + name)


def verify_reference(reference, level=1):
    if level not in (1, 2, 3, 4):
        raise ValueError('ROUTE / UNSUPPORTED REFERENCE LEVEL')
    for name, expected in pins(ROOT / f'manifest/gameplay-level{level}-reference.sha256').items():
        with (reference / name).open('rb') as stream:
            actual = hashlib.file_digest(stream, 'sha256').hexdigest()
        if actual != expected:
            raise ValueError('ROUTE / REFERENCE PIN ' + name)


def replay(run, schedule, reference=None, through_level=1):
    if through_level not in (1, 2, 3, 4):
        raise ValueError('ROUTE / UNSUPPORTED LAST LEVEL')
    rows = inputs(schedule)
    transitions = {len(rows): 2}
    for target in range(2, through_level + 1):
        rows += inputs(ROOT / f'tools/gameplay_routes/level{target}.tsv')
        transitions[len(rows)] = target + 1
    if reference:
        verify_reference(reference, through_level)
    initial = wait(run, 0, 600)
    if initial.get('level') != '1' or initial.get('reason') != 'first-frame':
        raise ValueError('ROUTE / INITIAL CHECKPOINT')
    if reference:
        compare(run, reference, 0)
    level = 1
    for sequence, (count, keys) in enumerate(rows, 1):
        previous = status(run)
        temporary = run / 'route-command.tmp'
        temporary.write_text(f'{sequence} {count} {keys}\n')
        temporary.replace(run / 'route-command.txt')
        current = wait(run, sequence, 180)
        if sequence in transitions:
            level = transitions[sequence]
            if current.get('reason') != 'next-level' or current.get('level') != str(level):
                raise ValueError('ROUTE / NEXT LEVEL NOT REACHED')
        elif (current.get('reason') != 'completed-frames' or
                current.get('level') != str(level) or
                int(current['frames']) - int(previous['frames']) != count):
            raise ValueError('ROUTE / UNEXPECTED CHECKPOINT')
        if reference:
            compare(run, reference, sequence)
        print(f'ROUTE CHECKPOINT {sequence}/{len(rows)} '
              f'level={current["level"]} step={current["step"]}', flush=True)
    if through_level >= 2:
        sequence = len(rows) + 1
        temporary = run / 'route-command.tmp'
        temporary.write_text(f'{sequence} finish\n')
        temporary.replace(run / 'route-command.txt')
        final = wait(run, sequence, 30)
        if final.get('reason') != 'discovery-finished' or final.get('level') != str(through_level + 1):
            raise ValueError('ROUTE / DISCOVERY FINISH NOT REACHED')
    print(f'ROUTE INPUT REPLAY COMPLETE checkpoints={len(rows)}', flush=True)
    # run_reference.py independently requires normal emulator exit, the unique
    # completion marker and the existing startup/display evidence.


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--schedule', type=Path,
                        default=ROOT / 'tools/gameplay_routes/level1.tsv')
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--through-level', type=int, choices=(1, 2, 3, 4), default=1)
    args = parser.parse_args()
    replay(args.run, args.schedule, args.reference, args.through_level)
