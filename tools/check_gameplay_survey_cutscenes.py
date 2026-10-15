"""Check visible cutscene evidence within the completed assisted entry survey."""
import argparse
import hashlib
from pathlib import Path

from check_gameplay_opening import compare_capture
from check_gameplay_route import check as check_route
from extract_original_data import ROOT, pins
from gameplay_assisted_inputs import REFERENCE


def check(native, reference, *, expanded=False):
    manifest = "gameplay-survey-cutscenes-expanded.sha256" if expanded else "gameplay-survey-cutscenes.sha256"
    for name, digest in pins(ROOT / 'manifest' / manifest).items():
        assert hashlib.sha256((reference / Path(name).name).read_bytes()).hexdigest() == digest, 'CUTSCENES / ORIGINAL PIN'
    original_log = (reference / 'session.log').read_text()
    assert 'FAIL REFERENCE' not in original_log, 'CUTSCENES / ORIGINAL FAILURE'
    assert 'CUTSCENE_OBSERVER_COMPLETE calls=6\n' in original_log, 'CUTSCENES / ORIGINAL CALLS'
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in original_log, 'CUTSCENES / ORIGINAL COMPLETION'
    assert (reference / 'route-actions.txt').read_bytes() == (REFERENCE / 'route-actions.txt').read_bytes(), 'CUTSCENES / ROUTE INPUTS'
    check_route(native, REFERENCE, 'a1200-030', level=0)
    log = (native / 'gdb-out.log').read_text()
    marker = 'CUTSCENE_RETURN ordinal=3 selector=10 result=0 copy=7 text=15\n'
    assert log.count(marker) == original_log.count(marker) == 1, 'CUTSCENES / DISCOVERY RETURN'
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    labels = ['nis-3-10-text-8', 'nis-3-10-return-1']
    if expanded:
        for marker in ('CUTSCENE_RETURN ordinal=4 selector=12 result=0 copy=1 text=4\n',
                       'CUTSCENE_RETURN ordinal=5 selector=2 result=0 copy=7 text=0\n'):
            assert log.count(marker) == original_log.count(marker) == 1, 'CUTSCENES / EXPANDED RETURN'
        labels += ['nis-4-12-text-4', 'nis-4-12-return-1',
                   'nis-5-2-copy-4', 'nis-5-2-return-1']
    for label in labels:
        args = [(native / (label + '-screen.bin')).read_bytes(),
                (native / (label + '-colors.bin')).read_bytes(), transfer,
                (reference / (label + '.indices')).read_bytes(),
                (reference / (label + '.rgb')).read_bytes()]
        compare_capture(*args)
        actual = (native / (label + '-a5.bin')).read_bytes()
        expected = (reference / (label + '-a5.bin')).read_bytes()
        assert len(actual) == len(expected) == 22266, 'CUTSCENES / A5 EXTENT'
        for distance, size in ((0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2), (0x245c, 2), (0x5208, 372)):
            assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'CUTSCENES / RNG ACTORS AND SCENE STATE'
    print('CUTSCENES PASS:', 'Discovery, tree index 1 and temple' if expanded else 'Discovery',
          'normal returns, illustrated and return pixels/colors/state; complete paired survey')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('--expanded', action='store_true')
    args = parser.parse_args()
    check(args.native, args.reference, expanded=args.expanded)
