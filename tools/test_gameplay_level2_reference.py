"""Reject corrupted evidence after a positive two-run original route check."""
import argparse
import contextlib
import io
from pathlib import Path
import shutil
import tempfile

from check_gameplay_level2_reference import check
from replay_gameplay_route import ROOT


def controls(first, second, level=2):
    check(first, second, level)
    last = 114 if level == 2 else 180
    rejected = 0
    with tempfile.TemporaryDirectory(prefix='level2-controls-', dir=ROOT / 'tmp') as directory:
        fixture = Path(directory)
        for name in ('session.log', 'route-actions.txt'):
            shutil.copyfile(second / name, fixture / name)
        for i in range(last + 2):
            for suffix in ('-a5.bin', '-context.bin', '.indices', '.rgb'):
                name = f'route-{i}{suffix}'
                shutil.copyfile(second / name, fixture / name)
        with contextlib.redirect_stdout(io.StringIO()):
            check(first, fixture, level)

        def reject(name, data):
            nonlocal rejected
            path = fixture / name
            good = path.read_bytes()
            assert data != good, 'LEVEL TWO / CONTROL MUST CHANGE EVIDENCE'
            path.write_bytes(data)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    check(first, fixture, level)
            except (AssertionError, ValueError):
                rejected += 1
            else:
                raise AssertionError('LEVEL TWO / CORRUPTION ACCEPTED ' + name)
            finally:
                path.write_bytes(good)

        log = (fixture / 'session.log').read_bytes()
        marker = b'GAMEPLAY_DISCOVERY_COMPLETE\n'
        reject('session.log', log.replace(marker, b''))
        reject('session.log', log + marker)
        reject('session.log', log + b'FAIL REFERENCE authored control\n')
        reject('session.log', log + b'GAMEPLAY_STATE_LOAD authored control\n')
        reject('session.log', log.replace(b'sequence=114 frames=1360 step=1360 level=3', b'sequence=114 frames=1360 step=1360 level=2'))
        reject('session.log', log.replace(f'sequence={last + 1} frames='.encode(), f'sequence={last + 2} frames='.encode()))
        reject('session.log', log.replace(b'frame=1360 step=1360 map=', b'frame=1359 step=1359 map='))
        reject('session.log', log.replace(b'sequence=96 frames=1031', b'sequence=96 frames=1032'))
        reject('route-actions.txt', (fixture / 'route-actions.txt').read_bytes().replace(f'{last + 1} finish'.encode(), f'{last + 1} 1 none'.encode()))
        for suffix, offset in (('-context.bin', 0x2100), ('.indices', 48 * 640 + 65),
                               ('.rgb', 13), ('-a5.bin', 22266 - 0x5208),
                               ('-a5.bin', 22266 - 0x18fc),
                               ('-a5.bin', 22266 - 0x4fc2)):
            name = 'route-100' + suffix
            data = bytearray((fixture / name).read_bytes())
            data[offset] ^= 1
            reject(name, data)
        reject('route-114-context.bin', (fixture / 'route-114-context.bin').read_bytes()[:-1])
        if level == 3:
            reject('session.log', log.replace(b'GAMEPLAY_ROUTE_NIS_RETURN selector=10\n', b''))
            reject('session.log', log + b'GAMEPLAY_ROUTE_NIS_ENTER selector=10\n')
            reject('session.log', log.replace(b'sequence=180 frames=2767 step=2767 level=4', b'sequence=180 frames=2767 step=2767 level=3'))
            reject('session.log', log.replace(b'frame=2767 step=2767 map=', b'frame=2766 step=2766 map='))
            for suffix, offset in (('-context.bin', 0x2100), ('.indices', 48 * 640 + 65), ('.rgb', 13), ('-a5.bin', 22266 - 0x18fc)):
                name = 'route-170' + suffix
                data = bytearray((fixture / name).read_bytes())
                data[offset] ^= 1
                reject(name, data)
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                check(first, first, level)
        except AssertionError:
            rejected += 1
        else:
            raise AssertionError('LEVEL TWO / SAME RUN ACCEPTED TWICE')
    print(f'LEVEL {level} REJECTION CONTROLS PASS: {rejected} corruptions rejected')
    return rejected


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('first', type=Path)
    parser.add_argument('second', type=Path)
    parser.add_argument('--level', type=int, choices=(2, 3), default=2)
    args = parser.parse_args()
    controls(args.first, args.second, args.level)
