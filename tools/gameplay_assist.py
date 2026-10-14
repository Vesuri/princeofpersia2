"""Send an explicitly logged assistance command to an opted-in reference run."""
import argparse
from pathlib import Path
from replay_gameplay_route import status, wait

LIMITS = {'shadow': (1, 1), 'health': (1, 12), 'level': (1, 14), 'x': (230, 720), 'room': (1, 32), 'y': (106, 346)}


def assist(run, kind, value):
    low, high = LIMITS[kind]
    if not low <= value <= high:
        raise ValueError('ASSIST / VALUE RANGE')
    if kind == 'y' and value not in (106, 226, 346):
        raise ValueError('ASSIST / FLOOR Y')
    before = status(run)
    if not before:
        raise ValueError('ASSIST / NO STOPPED CHECKPOINT')
    if kind != 'level':
        actor=(run/(before['prefix']+'-a5.bin')).read_bytes()
        if int.from_bytes(actor[-0x50b4:-0x50b2],'big') == 0:
            raise ValueError('ASSIST / ACTOR DEAD; restore a checkpoint or request a level restart')
    sequence = int(before['sequence']) + 1
    temporary = run / 'route-command.tmp'
    temporary.write_text(f'{sequence} assist {kind} {value}\n')
    temporary.replace(run / 'route-command.txt')
    after = wait(run, sequence, 60)
    # The pre-write capture publishes the same sequence briefly. Wait for the
    # after-write record rather than reporting that intermediate record as done.
    import time
    deadline = time.monotonic() + 60
    while after.get('reason') == 'assist-before':
        if time.monotonic() >= deadline:
            raise TimeoutError('ASSIST / AFTER CAPTURE MISSING; inspect existing run')
        time.sleep(.2)
        after = status(run)
    if after.get('reason') != 'assisted':
        raise ValueError('ASSIST / COMMAND DID NOT COMPLETE')
    print(after)
    return after


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('kind', choices=LIMITS)
    parser.add_argument('value', type=int)
    args = parser.parse_args()
    assist(args.run, args.kind, args.value)
