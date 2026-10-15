"""Compare the ship cinematic and the containing pinned level-one route."""
import argparse
import hashlib
from pathlib import Path

from check_gameplay_opening import compare_capture
from check_gameplay_route import check as check_route
from extract_original_data import ROOT, pins


def check(native, reference):
    for name, digest in pins(ROOT / 'manifest/gameplay-ship-reference.sha256').items():
        assert hashlib.sha256((reference / Path(name).name).read_bytes()).hexdigest() == digest, 'SHIP / ORIGINAL PIN'
    original = (reference / 'session.log').read_text()
    assert 'FAIL REFERENCE' not in original, 'SHIP / ORIGINAL FAILURE'
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in original and 'CUTSCENE_OBSERVER_COMPLETE calls=3\n' in original, 'SHIP / ORIGINAL COMPLETION'
    canonical = ROOT / 'ref/mame/runs/gameplay-level1-complete-1'
    keys = lambda text: [line for line in text.splitlines() if line.startswith('GAMEPLAY_ROUTE_KEYS ')]
    assert keys(original) == keys((canonical / 'session.log').read_text()), 'SHIP / REFERENCE WORKLOAD'
    log = (native / 'gdb-out.log').read_text()
    marker = 'CUTSCENE_RETURN ordinal=3 selector=9 result=0 copy=13 text=8\n'
    assert log.count(marker) == original.count(marker) == 1, 'SHIP / NORMAL RETURN'
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    for suffix in ('fade-1', 'text-4', 'copy-8', 'return-1'):
        label = 'nis-3-9-' + suffix
        compare_capture((native / (label + '-screen.bin')).read_bytes(),
                        (native / (label + '-colors.bin')).read_bytes(), transfer,
                        (reference / (label + '.indices')).read_bytes(),
                        (reference / (label + '.rgb')).read_bytes(),
                        black_transition=suffix == 'return-1')
        actual = (native / (label + '-a5.bin')).read_bytes()
        expected = (reference / (label + '-a5.bin')).read_bytes()
        assert len(actual) == len(expected) == 22266, 'SHIP / A5 EXTENT'
        for distance, size in ((0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2), (0x245c, 2), (0x5208, 372)):
            assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'SHIP / CAPTURE STATE'
    # The canonical route pins also establish the input workload. Its existing
    # D11 exception list remains exact; this adds no new exceptions.
    check_route(native, canonical,
                'a1200-030', level=1, deferred_rng=True)
    print('SHIP PAIR PASS: four cinematic checkpoints, normal return and completed level-one route')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.native, args.reference)
