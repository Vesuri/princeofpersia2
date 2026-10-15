"""Check fresh original Hall read/show/dismiss evidence; native pairing is separate."""
import argparse
import hashlib
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
    p.add_argument('--controls', action='store_true')
    a = p.parse_args()
    check(a.reference, a.repeat)
    if a.controls:
        controls(a.reference)
