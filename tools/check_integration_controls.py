"""Reject incomplete or altered native integration evidence."""
import argparse
import re
import shutil
import tempfile
from pathlib import Path
from check_integration_result import check
from extract_original_data import ROOT


def evidence_controls(run, reference, title_reference):
    check(run, reference, title_reference)
    scratch = Path(tempfile.mkdtemp(prefix='integration-controls-', dir=ROOT/'tmp'))
    names = ['gdb-out.log', 'logs/debug.uae', 'dh1/Shell.done',
             *[p.name for p in run.glob('*.bin')]]
    for name in names:
        target = scratch/name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(run/name, target)

    def flip(offset):
        return lambda data: data[:offset] + bytes([data[offset] ^ 1]) + data[offset+1:]

    changes = [
        ('gdb-out.log', lambda b: b.replace(b'INTEGRATION / ROUTE COMPLETE', b'INTEGRATION / INCOMPLETE')),
        ('gdb-out.log', lambda b: re.sub(rb'^INTEGRATION STAGE 2[^\n]*\n', b'', b, flags=re.M)),
        ('gdb-out.log', lambda b: re.sub(rb'^INTEGRATION STAGE 3[^\n]*\n', b'', b, flags=re.M)),
        ('gdb-out.log', lambda b: re.sub(rb'right_reads=\d+', b'right_reads=0', b)),
        ('gdb-out.log', lambda b: re.sub(rb'^INTEGRATION SPACE[^\n]*\n', b'', b, flags=re.M)),
        ('gdb-out.log', lambda b: re.sub(rb'INTEGRATION SPACE events=\d+', b'INTEGRATION SPACE events=0', b)),
        ('gdb-out.log', lambda b: b.replace(b'code=1 input=1', b'code=0 input=1')),
        ('gdb-out.log', lambda b: b.replace(b'code=1 input=1', b'code=1 input=0')),
        ('gdb-out.log', lambda b: b.replace(b'INTEGRATION RESTORED\n', b'')),
        ('gdb-out.log', lambda b: b.replace(b'heap=1', b'heap=0')),
        ('dh1/Shell.done', lambda b: b'SHELL PASS\n'),
        ('gdb-out.log', lambda b: b.replace(b'phase=16 cue=114', b'phase=17 cue=114')),
        ('title-screen.bin', flip(48*640+65)),
        ('title-colors.bin', flip(12)),
        ('first-room-screen.bin', flip(48*640+65)),
        ('first-room-colors.bin', flip(12)),
        ('first-room-planes.bin', flip(0)),
        ('first-room-copper.bin', flip(3)),
        ('first-room-a5.bin', flip(22266-0x18fc)),
        ('first-room-context.bin', flip(0x2180)),
    ]
    for name, mutate in changes:
        path = scratch/name
        good = path.read_bytes()
        bad = mutate(good)
        assert bad != good, 'INTEGRATION / INACTIVE CONTROL '+name
        path.write_bytes(bad)
        try:
            check(scratch, reference, title_reference)
        except (AssertionError, RuntimeError, ValueError):
            pass
        else:
            raise AssertionError('INTEGRATION / ACCEPTED BAD EVIDENCE '+name)
        finally:
            path.write_bytes(good)
    check(scratch, reference, title_reference)
    print(f'INTEGRATION EVIDENCE CONTROLS PASS: {len(changes)} rejected alterations')
    return len(changes)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('title_reference', type=Path)
    args = parser.parse_args()
    evidence_controls(args.run, args.reference, args.title_reference)
