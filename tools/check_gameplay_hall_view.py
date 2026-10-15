"""Check fresh original Hall read/show/dismiss evidence; native pairing is separate."""
import argparse
import hashlib
import json
import shutil
import tempfile
from pathlib import Path
from extract_original_data import ROOT, pins
from check_gameplay_hall import rows


def check(reference, repeat=None):
    expected = pins(ROOT/'manifest/gameplay-hall-reference.sha256')['saved-Prince.hof.data']
    imported = (reference/'imported-Prince.hof.data').read_bytes()
    assert hashlib.sha256(imported).hexdigest() == expected, 'HALL VIEW / IMPORT PIN'
    assert rows(imported) == [(b'codex', 75)], 'HALL VIEW / SAVED ROW'
    table = (reference/'hall-view-table.bin').read_bytes()
    assert rows(table) == rows(imported), 'HALL VIEW / RELOADED ROW'
    log = (reference/'session.log').read_text()
    previous = -1
    for marker in ('GAMEPLAY_HALL_VIEW_SHOWN', 'GAMEPLAY_HALL_VIEW_RETURN',
                   'GAMEPLAY_HALL_VIEW_PAUSED', 'GAMEPLAY_HALL_VIEW_RESUMED', 'GAMEPLAY_HALL_VIEW_COMPLETE'):
        assert log.count(marker+'\n') == 1, 'HALL VIEW / ENDPOINT '+marker
        current = log.index(marker+'\n')
        assert current > previous, 'HALL VIEW / ORDER'
        previous = current
    assert 'FAIL REFERENCE' not in log and 'REF TIMEOUT' not in log, 'HALL VIEW / FAILURE'
    from PIL import Image
    with Image.open(reference/'hall-view.ppm') as view:
        assert view.size == (640, 480), 'HALL VIEW / IMAGE EXTENT'
        assert view.convert('RGB').getcolors(maxcolors=2) is None, 'HALL VIEW / EMPTY IMAGE'
    if repeat:
        check(repeat)
        for name in ('hall-view-table.bin', 'hall-view.ppm'):
            assert (reference/name).read_bytes() == (repeat/name).read_bytes(), 'HALL VIEW / REPEAT '+name
    print('HALL VIEW ORIGINAL PASS: persisted row, shown table, dialog return and resumed gameplay; native not checked')


def check_native(reference, native):
    check(reference)
    from check_core_result import restored
    log = (native/'gdb-out.log').read_text()
    restored(native, log, 'a1200-030')
    assert 'HALL VIEW INVALID' not in log and 'Program received signal' not in log, 'HALL VIEW / NATIVE INTERRUPTION'
    previous = -1
    for marker in ('HALL VIEW TABLE count=1', 'HALL VIEW SHOWN id=7005',
                   'HALL VIEW FADED IN', 'HALL VIEW RETURN', 'HALL VIEW PAUSED', 'HALL VIEW RESUMED',
                   'HALL VIEW KEY REQUESTS 2',
                   'CORE REASON GAMEPLAY / HALL VIEW COMPLETE', 'HALL VIEW RESTORED'):
        assert log.count(marker+'\n') == 1, 'HALL VIEW / NATIVE ENDPOINT '+marker
        current = log.index(marker+'\n')
        assert current > previous, 'HALL VIEW / NATIVE ORDER'
        previous = current
    assert 'INTEGRATION AUDIT code=1 input=1\n' in log, 'HALL VIEW / NATIVE AUDIT'
    receipt = json.loads((native/'build-receipt.json').read_text())
    assert {'-DPOP2_VERIFY', '-DPOP2_GAMEPLAY_HALL_VIEW_PROBE',
            '-DPOP2_GAMEPLAY_RESTART_PROBE', '-DPOP2_INTEGRATION_PROBE'}.issubset(
                receipt['flags'].split()), 'HALL VIEW / NATIVE FLAGS'
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        assert hashlib.sha256((native/name).read_bytes()).hexdigest() == receipt[key], 'HALL VIEW / NATIVE BUILD'
    assert (native/'dh1/Data/Prince.hof').read_bytes() == (reference/'imported-Prince.hof.data').read_bytes(), 'HALL VIEW / NATIVE SAVED FILE'
    assert (native/'hall-view-table.bin').read_bytes() == (reference/'hall-view-table.bin').read_bytes(), 'HALL VIEW / NATIVE RELOADED TABLE'
    image = (native/'hall-ui-screen.bin').read_bytes()
    assert len(image) == 640*480 and len(set(image)) > 1, 'HALL VIEW / NATIVE EMPTY UI'
    print('HALL VIEW NATIVE PASS: original-written table, owned UI, dismissal, resumed frame and restoration')


def native_controls(reference, native):
    check_native(reference, native)
    names = ('gdb-out.log', 'dh1/Shell.done', 'logs/debug.uae',
             'build-receipt.json', 'dh1/PrinceOfPersia2', 'PrinceOfPersia2.elf',
             'dh1/Data/Prince.hof', 'hall-view-table.bin', 'hall-ui-screen.bin')
    mutations = (
        ('gdb-out.log', lambda b: b.replace(b'HALL VIEW RESUMED\n', b'')),
        ('gdb-out.log', lambda b: b.replace(b'HALL VIEW KEY REQUESTS 2', b'HALL VIEW KEY REQUESTS 1')),
        ('logs/debug.uae', lambda b: b.replace(b'cpu_model=68030', b'cpu_model=68040')),
        ('dh1/Shell.done', lambda b: b'SHELL PASS\n'),
        ('hall-view-table.bin', lambda b: b[:2]+b'X'+b[3:]),
        ('hall-ui-screen.bin', lambda b: bytes(len(b))),
        ('build-receipt.json', lambda b: b.replace(b'-DPOP2_GAMEPLAY_HALL_VIEW_PROBE', b'-DINVALID')),
    )
    with tempfile.TemporaryDirectory(prefix='hall-view-native-controls-', dir=ROOT/'tmp') as scratch:
        target = Path(scratch)
        for name in names:
            (target/name).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(native/name, target/name)
        for name, mutate in mutations:
            path = target/name
            before = path.read_bytes()
            after = mutate(before)
            assert before != after, 'HALL VIEW / INEFFECTIVE CONTROL'
            path.write_bytes(after)
            try:
                check_native(reference, target)
            except (AssertionError, RuntimeError):
                pass
            else:
                raise AssertionError('HALL VIEW / NATIVE CONTROL ACCEPTED '+name)
            path.write_bytes(before)
    print('HALL VIEW NATIVE CONTROLS PASS: seven altered-evidence cases rejected')


def controls(reference):
    check(reference)
    with tempfile.TemporaryDirectory(prefix='hall-view-controls-', dir=ROOT/'tmp') as scratch:
        target = Path(scratch)
        names = ('imported-Prince.hof.data', 'hall-view-table.bin', 'session.log', 'hall-view.ppm')
        mutations = (
            ('imported-Prince.hof.data', lambda b: b[:-1]+bytes([b[-1]^1])),
            ('hall-view-table.bin', lambda b: b[:2]+b'X'+b[3:]),
            ('session.log', lambda b: b.replace(b'GAMEPLAY_HALL_VIEW_RETURN\n', b'')),
            ('session.log', lambda b: b.replace(b'GAMEPLAY_HALL_VIEW_RESUMED\n', b'')),
            ('session.log', lambda b: b.replace(b'GAMEPLAY_HALL_VIEW_COMPLETE\n', b'')),
            ('session.log', lambda b: b+b'FAIL REFERENCE injected\n'),
        )
        for name, mutate in mutations:
            for original in names:
                shutil.copyfile(reference/original, target/original)
            path = target/name
            path.write_bytes(mutate(path.read_bytes()))
            try:
                check(target)
            except AssertionError:
                pass
            else:
                raise AssertionError('HALL VIEW / CONTROL ACCEPTED '+name)
    print('HALL VIEW CONTROLS PASS: six altered-evidence cases rejected')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('reference', type=Path)
    p.add_argument('--repeat', type=Path)
    p.add_argument('--native', type=Path)
    p.add_argument('--controls', action='store_true')
    a = p.parse_args()
    check(a.reference, a.repeat)
    if a.controls:
        controls(a.reference)
    if a.native:
        check_native(a.reference, a.native)
    if a.native and a.controls:
        native_controls(a.reference, a.native)
