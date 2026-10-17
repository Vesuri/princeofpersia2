#!/usr/bin/env python3
"""Exercise fail-closed discovery restores against a running original Macintosh."""
import argparse
from pathlib import Path
import time
import shutil
import tempfile

from replay_gameplay_route import status, wait

# Cover neutral, differing held keys, identical held keys, and neutral again
# after a transition. No case claims that matching key names imply continuity.
COMMANDS = (
    'save neutral', 'load neutral', '1 left', 'save held', 'load held',
    'load neutral', '1 none', 'load held', 'load neutral', '1 none', 'finish',
)
REJECTIONS = {2: ('neutral', 'none', 'none'), 5: ('held', 'left', 'left'),
              6: ('neutral', 'none', 'left'), 8: ('held', 'left', 'none'),
              9: ('neutral', 'none', 'none')}
EXTENTS = {'-a5.bin': 22266, '-context.bin': 0x2200, '.indices': 640*480, '.rgb': 768}
SUFFIXES = tuple(EXTENTS)


def equal(run, a, b):
    for suffix in SUFFIXES:
        if (run / f'route-{a}{suffix}').read_bytes() != (run / f'route-{b}{suffix}').read_bytes():
            raise ValueError(f'RESTORE / REJECTION MUTATED {a}/{b}{suffix}')


def check(run):
    for i in range(12):
        for suffix, size in EXTENTS.items():
            if (run / f'route-{i}{suffix}').stat().st_size != size:
                raise ValueError('RESTORE / CAPTURE EXTENT')
    actions = (run / 'route-actions.txt').read_text().splitlines()
    expected = [f'{i} {command}' for i, command in enumerate(COMMANDS, 1)]
    if actions != expected:
        raise ValueError('RESTORE / COMMAND COVERAGE')
    reports = (run / 'route-state-rejections.txt').read_text().splitlines()
    expected_reports = [
        f'sequence={i} GAMEPLAY_STATE_LOAD_UNSUPPORTED slot={slot} saved_keys={saved} current_keys={current} reason=input-continuity-unverified'
        for i, (slot, saved, current) in REJECTIONS.items()]
    if reports != expected_reports:
        raise ValueError('RESTORE / REJECTION COVERAGE')
    log = (run / 'session.log').read_text()
    if (log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') != 1
            or 'FAIL REFERENCE' in log or 'GAMEPLAY_STATE_LOAD slot=' in log):
        raise ValueError('RESTORE / POSITIVE COMPLETION')
    for i in REJECTIONS:
        if f'reason=state-load-rejected prefix=route-{i}\n' not in log:
            raise ValueError('RESTORE / MISSING REJECTED CHECKPOINT')
        equal(run, i-1, i)
    for i in (1, 4):
        equal(run, i-1, i)
    final = status(run)
    if final.get('sequence') != '11' or final.get('reason') != 'discovery-finished' or final.get('frames') != '3':
        raise ValueError('RESTORE / CONTINUATION COMPLETION')
    # A live original completed three requested frames despite rejected loads.
    if (run/'route-10-a5.bin').read_bytes() == (run/'route-0-a5.bin').read_bytes():
        raise ValueError('RESTORE / NO GAME PROGRESS')
    print('PASS discovery restore: five reported rejections, unchanged checkpoints, three completed original frames, explicit finish')


def controls(run):
    check(run)
    mutations = (
        ('route-state-rejections.txt', lambda data: data.replace(b'current_keys=left', b'current_keys=none', 1)),
        ('route-actions.txt', lambda data: data.replace(b'5 load held', b'5 1 none', 1)),
        ('session.log', lambda data: data.replace(b'GAMEPLAY_DISCOVERY_COMPLETE', b'INCOMPLETE', 1)),
        ('route-6-context.bin', lambda data: bytes([data[0] ^ 1]) + data[1:]),
        ('route-status.txt', lambda data: data.replace(b'frames=3', b'frames=2', 1)),
    )
    # Copy only small evidence files, never disks or emulator states.
    with tempfile.TemporaryDirectory(prefix='restore-controls-') as temporary:
        copy = Path(temporary)
        names = ['route-state-rejections.txt', 'route-actions.txt', 'session.log', 'route-status.txt']
        names += [f'route-{i}{suffix}' for i in range(12) for suffix in SUFFIXES]
        for name in names:
            shutil.copyfile(run/name, copy/name)
        for name, mutate in mutations:
            target = copy/name
            original = target.read_bytes()
            changed = mutate(original)
            if changed == original:
                raise ValueError('RESTORE / CONTROL DID NOT MUTATE')
            target.write_bytes(changed)
            try:
                check(copy)
            except ValueError:
                pass
            else:
                raise ValueError('RESTORE / ACCEPTED CORRUPTED '+name)
            finally:
                target.write_bytes(original)
    print('PASS discovery restore: five evidence rejection controls')


def drive(run):
    initial = wait(run, 0, 600)
    if initial.get('reason') != 'first-frame':
        raise ValueError('RESTORE / INITIAL CHECKPOINT')
    for i, command in enumerate(COMMANDS, 1):
        temporary = run/'route-command.tmp'
        temporary.write_text(f'{i} {command}\n')
        temporary.replace(run/'route-command.txt')
        result = wait(run, i, 120)
        if i in REJECTIONS and result.get('reason') != 'state-load-rejected':
            raise ValueError('RESTORE / LOAD WAS NOT REJECTED')
    deadline = time.monotonic()+30
    while 'GAMEPLAY_DISCOVERY_COMPLETE\n' not in (run/'session.log').read_text():
        if time.monotonic() > deadline:
            raise ValueError('RESTORE / FINISH DEADLINE')
        time.sleep(.2)
    check(run)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--drive', action='store_true')
    mode.add_argument('--controls', action='store_true')
    args = parser.parse_args()
    (drive if args.drive else controls if args.controls else check)(args.run)
