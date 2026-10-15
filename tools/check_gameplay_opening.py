"""Check the unskipped Opening1 lifecycle and paired visible checkpoints."""
import argparse
import hashlib
import json
from pathlib import Path

from check_core_result import restored
from check_integration_result import crop
from extract_original_data import ROOT, pins


def compare_capture(screen, colors, transfer, indices, rgb):
    assert len(colors) == 2056 and len(transfer) == 256, 'OPENING / COLOR EXTENT'
    assert colors[4:8] == bytes.fromhex('800000ff'), 'OPENING / DEVICE COLOR TABLE'
    assert crop(screen) == crop(indices), 'OPENING / PIXELS'
    actual = bytes(transfer[colors[8 + i * 8 + component]]
                   for i in range(256) for component in (2, 4, 6))
    assert len(rgb) == 768 and actual == rgb, 'OPENING / DISPLAY COLORS'


def check(native, reference):
    for name, digest in pins(ROOT / 'manifest/gameplay-natural-opening.sha256').items():
        path = reference / Path(name).name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'OPENING / REFERENCE PIN'
    receipt = json.loads((native / 'build-receipt.json').read_text())
    expected = {'-DPOP2_' + flag for flag in
                ('CORE', 'DISPLAY', 'INPUT', 'UI', 'AUDIO', 'INTEGRATION_PROBE', 'VERIFY')}
    assert set(receipt['flags'].split()) == expected, 'OPENING / BUILD FLAGS'
    log = (native / 'gdb-out.log').read_text()
    restored(native, log, 'a1200-030')
    for marker in ('CUTSCENE_ENTER ordinal=1 selector=7',
                   'CUTSCENE_RETURN ordinal=1 selector=7 result=0 copy=5 text=21',
                   'INTEGRATION ESC reads=0',
                   'CORE REASON INTEGRATION / ROUTE COMPLETE',
                   'CORE EXCEPTION vector=0 pc=00000000',
                   'INTEGRATION AUDIT code=1 input=1', 'INTEGRATION RESTORED'):
        assert marker + '\n' in log, 'OPENING / LIFECYCLE ' + marker
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    for label in ('nis-1-7-text-8', 'nis-1-7-return-1'):
        args = [(native / (label + '-screen.bin')).read_bytes(),
                (native / (label + '-colors.bin')).read_bytes(), transfer,
                (reference / (label + '.indices')).read_bytes(),
                (reference / (label + '.rgb')).read_bytes()]
        compare_capture(*args)
        for field, offset in ((0, 48 * 640 + 65), (4, 0)):
            changed = list(args)
            data = bytearray(changed[field]); data[offset] ^= 1; changed[field] = bytes(data)
            try:
                compare_capture(*changed)
            except AssertionError:
                pass
            else:
                raise AssertionError('OPENING / CORRUPTION CONTROL ACCEPTED')
    print('OPENING PASS: selector 7 normal return, illustrated frame and return pixels/colors, '
          'no Escape, native completion/restoration; four corruption controls')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    args = parser.parse_args()
    check(args.native, args.reference)
