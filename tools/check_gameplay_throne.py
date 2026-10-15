"""Check the throne cutscene pair; report the containing route's separate outcome."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from check_core_result import restored
from check_gameplay_opening import compare_capture
from extract_original_data import ROOT, pins


def check(native, reference):
    for name, digest in pins(ROOT / 'manifest/gameplay-throne-reference.sha256').items():
        assert hashlib.sha256((reference / Path(name).name).read_bytes()).hexdigest() == digest, 'THRONE / REFERENCE PIN'
    original_log = (reference / 'session.log').read_text()
    assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in original_log and 'CUTSCENE_OBSERVER_COMPLETE calls=3\n' in original_log, 'THRONE / ORIGINAL COMPLETION'
    receipt = json.loads((native / 'build-receipt.json').read_text())
    assert {'-DPOP2_VERIFY', '-DPOP2_GAMEPLAY_ROUTE_PROBE', '-DPOP2_GAMEPLAY_ROUTE_LEVEL=0'}.issubset(receipt['flags'].split()), 'THRONE / BUILD FLAGS'
    for key, name in (('exe', 'dh1/PrinceOfPersia2'), ('elf', 'PrinceOfPersia2.elf')):
        assert hashlib.sha256((native / name).read_bytes()).hexdigest() == receipt[key], 'THRONE / BUILD IDENTITY'
    log = (native / 'gdb-out.log').read_text()
    restored(native, log, 'a1200-030')
    assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log and 'INTEGRATION AUDIT code=1 input=1\n' in log, 'THRONE / RESTORATION AUDITS'
    marker = 'CUTSCENE_RETURN ordinal=3 selector=6 result=0 copy=1 text=9\n'
    assert log.count(marker) == original_log.count(marker) == 1, 'THRONE / NATURAL RETURN'
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    for suffix in ('fade-1', 'text-4', 'text-8', 'return-1'):
        label = 'nis-3-6-' + suffix
        compare_capture((native / (label + '-screen.bin')).read_bytes(),
                        (native / (label + '-colors.bin')).read_bytes(), transfer,
                        (reference / (label + '.indices')).read_bytes(),
                        (reference / (label + '.rgb')).read_bytes())
        actual = (native / (label + '-a5.bin')).read_bytes()
        expected = (reference / (label + '-a5.bin')).read_bytes()
        assert len(actual) == len(expected) == 22266, 'THRONE / A5 EXTENT'
        for distance, size in ((0x18fc, 4), (0x4fd2, 2), (0x4fc2, 2), (0x245c, 2), (0x5208, 372)):
            assert actual[-distance:-distance+size] == expected[-distance:-distance+size], 'THRONE / CAPTURE STATE'
    reasons = re.findall(r'^CORE REASON (.+)$', log, re.M)
    assert len(reasons) == 1, 'THRONE / CONTAINING ROUTE OUTCOME'
    print('THRONE CUTSCENE PAIR PASS: four pixel/colour/state checkpoints and normal return')
    print('Containing route outcome (separate gate):', reasons[0])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.native, args.reference)
