"""Corrupt fresh Macintosh import evidence only after both originals pass."""
import argparse
import contextlib
import io
from pathlib import Path
import shutil
import tempfile
from check_gameplay_import import check_reference
from extract_original_data import ROOT, pins


def controls(first, second):
    assert first.resolve() != second.resolve(), 'IMPORT / TWO INDEPENDENT RUNS REQUIRED'
    check_reference(first)
    check_reference(second)
    rejected = 0
    with tempfile.TemporaryDirectory(prefix='import-reference-controls-', dir=ROOT / 'tmp') as directory:
        fixture = Path(directory)
        names = {'session.log', 'opening.ppm', 'environment.json', 'checkpoint.json'}
        names.update(pins(ROOT / 'manifest/gameplay-import-frames.sha256'))
        for name in names:
            if (second / name).exists():
                shutil.copyfile(second / name, fixture / name)
        with contextlib.redirect_stdout(io.StringIO()):
            check_reference(fixture)

        def reject(name, data):
            nonlocal rejected
            path = fixture / name
            good = path.read_bytes()
            assert good != data, 'IMPORT / CONTROL MUST CHANGE EVIDENCE'
            path.write_bytes(data)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    check_reference(fixture)
            except (AssertionError, ValueError, RuntimeError):
                rejected += 1
            else:
                raise AssertionError('IMPORT / CORRUPTION ACCEPTED ' + name)
            finally:
                path.write_bytes(good)

        log = (fixture / 'session.log').read_bytes()
        marker = b'GAMEPLAY_REFERENCE_COMPLETE route=load-import loaded_frames=1\n'
        reject('session.log', log.replace(marker, b''))
        reject('session.log', log + marker)
        reject('session.log', log + b'FAIL REFERENCE authored control\n')
        reject('session.log', log + b'GAMEPLAY_FILE_ENTER save-enter\n')
        reject('session.log', log.replace(b'command=01910004', b'command=01910005'))
        reject('session.log', log.replace(b'step=14 left=8', b'step=14 left=0'))
        reject('session.log', log.replace(b'label=load-enter step=23', b'label=load-enter step=24'))
        reject('session.log', log.replace(b'GAMEPLAY_VOLUME_FLUSHED\n', b''))
        reject('session.log', log.replace(b'GAMEPLAY_LOADED_FRAME result=FFFE', b'GAMEPLAY_LOADED_FRAME result=0000'))
        for name, offset in [('gameplay-first-frame-2-a5.bin', 22266 - 0x18fc),
                             ('gameplay-loaded-frame-context.bin', 0x2180),
                             ('gameplay-loaded-frame.indices', 48 * 640 + 65),
                             ('gameplay-loaded-frame.rgb', 13), ('opening.ppm', 2000)]:
            data = bytearray((fixture / name).read_bytes()); data[offset] ^= 1
            reject(name, data)
    print(f'IMPORT REFERENCE CONTROLS PASS: {rejected} corruptions rejected')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('first', type=Path); p.add_argument('second', type=Path)
    a = p.parse_args(); controls(a.first, a.second)
