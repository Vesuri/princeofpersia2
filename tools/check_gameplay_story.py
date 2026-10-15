"""Compare a pinned timed story or princess cinematic and its assisted route."""
import argparse
from pathlib import Path
import re

from check_gameplay_opening import compare_capture
from check_gameplay_route import check as check_route
from gameplay_assisted_inputs import metadata
from extract_original_data import pins


def check(native, reference, manifest, selector):
    metadata(reference, manifest)  # Pins, positive completion and logged writes.
    original = (reference / 'session.log').read_text()
    log = (native / 'gdb-out.log').read_text()
    returns = re.findall(
        rf'^CUTSCENE_RETURN ordinal=(\d+) selector={selector} result=0 copy=(\d+) text=(\d+)$',
        original, re.M)
    assert len(returns) == 1, 'STORY / ORIGINAL NORMAL RETURN'
    ordinal, copies, texts = returns[0]
    marker = f'CUTSCENE_RETURN ordinal={ordinal} selector={selector} result=0 copy={copies} text={texts}\n'
    assert log.count(marker) == 1, 'STORY / NATIVE NORMAL RETURN'
    prefix = f'nis-{ordinal}-{selector}-'
    pinned = pins(manifest)
    labels = sorted(Path(name).stem for name in pinned
                    if name.startswith(prefix) and name.endswith('.indices'))
    required = ('copy-1','copy-2','return-1') if selector == 5 else ('fade-1','return-1')
    assert all(prefix + suffix in labels for suffix in required), 'STORY / CAPTURE COVERAGE'
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    for label in labels:
        assert all(label + suffix in pinned for suffix in ('.indices', '.rgb', '-a5.bin')), 'STORY / CAPTURE PINS'
        compare_capture((native / (label + '-screen.bin')).read_bytes(),
                        (native / (label + '-colors.bin')).read_bytes(), transfer,
                        (reference / (label + '.indices')).read_bytes(),
                        (reference / (label + '.rgb')).read_bytes())
        actual = (native / (label + '-a5.bin')).read_bytes()
        expected = (reference / (label + '-a5.bin')).read_bytes()
        assert len(actual) == len(expected) == 22266, 'STORY / A5 EXTENT'
        for distance, size in ((0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2),
                               (0x245c, 2), (0x5208, 372)):
            assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'STORY / CAPTURE STATE'
    check_route(native, reference, 'a1200-030', level=0, manifest=manifest)
    print(f'CINEMATIC {selector} PAIR PASS: {len(labels)} cinematic checkpoints, normal return and completed assisted route')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--selector', type=int, choices=[5, *range(12, 21)], required=True)
    args = parser.parse_args()
    check(args.native, args.reference, args.manifest, args.selector)
