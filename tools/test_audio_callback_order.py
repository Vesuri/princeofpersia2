"""Sabotage completed callback-order evidence; every corruption must reject."""
import argparse
from pathlib import Path
import shutil
import tempfile
from check_audio_callback_order import check, original_rows
from extract_original_data import ROOT


def controls(run, reference):
    check(run, reference)
    original = (reference / 'session.log').read_text()
    original_rows(original)
    for changed in (original.replace('AUDIO_CALLBACK_ORDER_COMPLETE calls=6', 'MISSING'),
                    original.replace('callbacks=1 result=00000001', 'callbacks=2 result=00000001'),
                    original.replace('index=4 id=7 callbacks=1 result=00000000',
                                     'index=4 id=7 callbacks=1 result=00000001')):
        assert changed != original
        try:
            original_rows(changed)
        except ValueError:
            pass
        else:
            raise AssertionError('AUDIO ORDER / ACCEPTED BAD ORIGINAL')
    names = ('gdb-out.log', 'audio-events.bin', 'build-receipt.json',
             'PrinceOfPersia2.elf', 'dh1/PrinceOfPersia2', 'dh1/Shell.done', 'logs/debug.uae')
    def flip(data, offset):
        result = bytearray(data); result[offset] ^= 1; return bytes(result)
    changes = [('gdb-out.log', lambda b: b.replace(b'CALLBACK ORDER COMPLETE', b'INCOMPLETE')),
               ('gdb-out.log', lambda b: b.replace(b'abi=7', b'abi=6')),
               ('gdb-out.log', lambda b: b.replace(b'heap=1', b'heap=0')),
               ('logs/debug.uae', lambda b: b.replace(b'sound_output=exact', b'sound_output=none')),
               ('build-receipt.json', lambda b: b.replace(b'-DPOP2_AUDIO_ORDER_PROBE', b'-DWRONG_PROBE')),
               ('dh1/Shell.done', lambda b: b'SHELL PASS\n'),
               ('PrinceOfPersia2.elf', lambda b: flip(b, 32)),
               ('audio-events.bin', lambda b: b[:-24])]
    # The callback's parameter mutation, the actual active ID, callback counts,
    # status queries and every-call ABI counter are independently checked.
    for offset in (3 * 24 + 4, 3 * 24 + 8, 3 * 24 + 12, 3 * 24 + 16,
                   4 * 24, 5 * 24, 2 * 24 + 20):
        changes.append(('audio-events.bin', lambda b, offset=offset: flip(b, offset)))
    with tempfile.TemporaryDirectory(prefix='audio-order-controls-', dir=ROOT / 'tmp') as directory:
        candidate = Path(directory)
        for name in names:
            target = candidate / name; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(run / name, target)
        check(candidate, reference)
        for name, change in changes:
            path = candidate / name; good = path.read_bytes(); bad = change(good)
            assert bad != good
            path.write_bytes(bad)
            try:
                check(candidate, reference)
            except (ValueError, RuntimeError, AssertionError):
                pass
            else:
                raise AssertionError('AUDIO ORDER / ACCEPTED BAD EVIDENCE ' + name)
            finally:
                path.write_bytes(good)
    print('AUDIO CALLBACK ORDER CONTROLS PASS:', len(changes) + 3)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path); parser.add_argument('reference', type=Path)
    args = parser.parse_args(); controls(args.run, args.reference)
