"""Check the unskipped Opening1 lifecycle and paired visible checkpoints."""
import argparse
import hashlib
import json
from pathlib import Path

from check_core_result import restored
from check_integration_result import crop
from extract_original_data import ROOT, pins


def compare_capture(screen, colors, transfer, indices, rgb, *, black_transition=False):
    assert len(colors) == 2056 and len(transfer) == 256, 'OPENING / COLOR EXTENT'
    assert colors[4:8] == bytes.fromhex('800000ff'), 'OPENING / DEVICE COLOR TABLE'
    actual = bytes(transfer[colors[8 + i * 8 + component]]
                   for i in range(256) for component in (2, 4, 6))
    assert len(rgb) == 768 and actual == rgb, 'OPENING / DISPLAY COLORS'
    if black_transition:
        for pixels in (crop(screen), crop(indices)):
            assert all(actual[i*3:i*3+3] == bytes(3) for i in pixels), 'OPENING / BLACK TRANSITION'
    else:
        assert crop(screen) == crop(indices), 'OPENING / PIXELS'


def check(native, reference, *, attract=False):
    manifest = "gameplay-natural-attract.sha256" if attract else "gameplay-natural-opening.sha256"
    for name, digest in pins(ROOT / 'manifest' / manifest).items():
        path = reference / Path(name).name
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'OPENING / REFERENCE PIN'
    receipt = json.loads((native / 'build-receipt.json').read_text())
    expected = {'-DPOP2_' + flag for flag in
                ('CORE', 'DISPLAY', 'INPUT', 'UI', 'AUDIO', 'INTEGRATION_PROBE', 'VERIFY')}
    if attract:
        expected.add('-DPOP2_CUTSCENE_ATTRACT_PROBE')
    assert set(receipt['flags'].split()) == expected, 'OPENING / BUILD FLAGS'
    log = (native / 'gdb-out.log').read_text()
    restored(native, log, 'a1200-030')
    completion = 'GAMEPLAY / ATTRACT COMPLETE' if attract else 'INTEGRATION / ROUTE COMPLETE'
    for marker in ('CUTSCENE_ENTER ordinal=1 selector=7',
                   'CUTSCENE_RETURN ordinal=1 selector=7 result=0 copy=5 text=21',
                   'INTEGRATION ESC reads=0',
                   'CORE REASON ' + completion,
                   'CORE EXCEPTION vector=0 pc=00000000',
                   'INTEGRATION AUDIT code=1 input=1', 'INTEGRATION RESTORED'):
        assert marker + '\n' in log, 'OPENING / LIFECYCLE ' + marker
    labels = ['nis-1-7-text-8', 'nis-1-7-return-1']
    if attract:
        original_log = (reference / 'session.log').read_text()
        for marker in ('CUTSCENE_RETURN ordinal=2 selector=4 result=0 copy=1 text=0',
                       'CUTSCENE_RETURN ordinal=3 selector=8 result=0 copy=43 text=23'):
            assert marker + '\n' in log and marker + '\n' in original_log, 'OPENING / ATTRACT RETURN'
        assert 'INTEGRATION SPACE events=0\n' in log, 'OPENING / NO TITLE INPUT'
        assert 'GAMEPLAY_DISCOVERY_COMPLETE\n' in original_log, 'OPENING / ORIGINAL COMPLETION'
        assert 'CUTSCENE_OBSERVER_COMPLETE calls=3\n' in original_log, 'OPENING / ORIGINAL CALLS'
        context = (native / 'first-room-context.bin').read_bytes()
        assert len(context) == 0x2200 and context == (reference / 'route-0-context.bin').read_bytes(), 'OPENING / FIRST ROOM CONTEXT'
        a5 = (native / 'first-room-a5.bin').read_bytes()
        original_a5 = (reference / 'route-0-a5.bin').read_bytes()
        assert len(a5) == len(original_a5) == 22266, 'OPENING / A5 EXTENT'
        for distance, size in ((0x4fd2, 2), (0x4fc2, 2), (0x18fc, 4), (0x5208, 372)):
            assert a5[-distance:-distance+size] == original_a5[-distance:-distance+size], 'OPENING / FIRST ROOM STATE'
        labels += ['nis-2-4-return-1', 'nis-3-8-copy-8', 'nis-3-8-copy-32',
                   'nis-3-8-text-8', 'nis-3-8-return-1']
    transfer = (native / 'dh1/video-transfer.bin').read_bytes()
    for label in labels:
        args = [(native / (label + '-screen.bin')).read_bytes(),
                (native / (label + '-colors.bin')).read_bytes(), transfer,
                (reference / (label + '.indices')).read_bytes(),
                (reference / (label + '.rgb')).read_bytes()]
        black_transition = label == 'nis-2-4-return-1'
        compare_capture(*args, black_transition=black_transition)
        for field, offset in ((0, 48 * 640 + 65), (4, 0)):
            changed = list(args)
            data = bytearray(changed[field])
            if black_transition and field == 0:
                data[offset] = next(i for i in range(256) if args[4][i*3:i*3+3] != bytes(3))
            else:
                data[offset] ^= 1
            changed[field] = bytes(data)
            try:
                compare_capture(*changed, black_transition=black_transition)
            except AssertionError:
                pass
            else:
                raise AssertionError('OPENING / CORRUPTION CONTROL ACCEPTED')
    print('OPENING PASS:', 'selectors 7, 4, 8' if attract else 'selector 7',
          'normal returns, paired pixels/colors, native completion/restoration;',
          len(labels) * 2, 'corruption controls')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('reference', type=Path)
    parser.add_argument('--attract', action='store_true')
    args = parser.parse_args()
    check(args.native, args.reference, attract=args.attract)
