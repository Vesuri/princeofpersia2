"""Compare lossless BankRLELR effects with original Macintosh execution."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from bank_skip_fixture import verify_fixture
from extract_original_data import ROOT, pins
from native_machine import check as check_machine


def rows(data, cases):
    at = 0
    result = []
    for index in range(len(cases)):
        for cc in range(32 if index < 14 else 1):
            assert at+128 <= len(data), 'BANK / SHORT HEADER'
            count = struct.unpack_from('>I', data, at+124)[0]
            end = at+128+count*5
            assert end <= len(data), 'BANK / SHORT PIXELS'
            previous = -1
            for pos in range(at+128, end, 5):
                offset, value = struct.unpack_from('>IB', data, pos)
                assert previous < offset < 307200, 'BANK / PIXEL ORDER'
                assert value != (offset*37+index*13)&255, 'BANK / REDUNDANT PIXEL'
                previous = offset
            result.append(data[at:end])
            at = end
    assert at == len(data), 'BANK / EXTRA OUTPUT'
    assert len(result) == 1468, 'BANK / CASE COUNT'
    return result


def compare(actual, expected, cases):
    actual_rows = rows(actual, cases)
    expected_rows = rows(expected, cases)
    for index, (a, b) in enumerate(zip(actual_rows, expected_rows)):
        assert a == b, ('BANK / EFFECT MISMATCH', index)


def check(reference, native):
    receipt = json.loads((native/'receipt.json').read_text())
    fixture = ROOT/receipt['fixture']
    verify_fixture(fixture)
    cases = json.loads((fixture/'cases.json').read_text())
    assert len(cases) == 1034
    check_machine(native, 'a1200-fast')
    assert receipt['image'] == hashlib.sha256((fixture/'original.bin').read_bytes()).hexdigest()
    assert receipt['native_source'] == hashlib.sha256((ROOT/'tools/bank_skip_native.c').read_bytes()).hexdigest()
    assert set(receipt['sources']) == {'tools/wrapper_ccr_fixture.s', 'tools/bank_skip_native.c'}
    for name, digest in receipt['sources'].items():
        assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest, name
    for name, key in [('fixture.elf', 'elf'), ('dh1/WrapperProbe', 'exe')]:
        assert hashlib.sha256((native/name).read_bytes()).hexdigest() == receipt[key]
    assert (native/'dh1/fixture.complete').read_text() == 'BANK_SKIP_COMPLETE cases=1468\n'
    assert (native/'dh1/Shell.done').read_text() == 'SHELL PASS\n'
    log = (reference/'session.log').read_text()
    assert log.count('BANK_COMPLETE cases=1468\n') == 1 and log.count('BANK_RESTORED\n') == 1
    assert 'FAIL REFERENCE' not in log and 'unmapped' not in log[log.index('BANK_ALLOC\n'):]
    expected = (reference/'bank.effects').read_bytes()
    for name in ('original.effects', 'replacement.effects'):
        compare((native/'dh1'/name).read_bytes(), expected, cases)
    # Every returned register, CCR/SP, frame locals, pixel location/value and extent.
    first = rows(expected, cases)[0]
    offsets = [*range(0, 68, 4), 60, 61, *range(68, 124, 4), 124]
    if len(first) > 128:
        offsets += [128, 132]
    rejected = 0
    for offset in offsets:
        changed = bytearray(expected); changed[offset] ^= 1
        try:
            compare(changed, expected, cases)
        except AssertionError:
            rejected += 1
        else:
            raise AssertionError(('BANK / INACTIVE CONTROL', offset))
    for changed in (expected[:-1], expected+b'\0'):
        try:
            compare(changed, expected, cases)
        except AssertionError:
            rejected += 1
        else:
            raise AssertionError('BANK / INACTIVE EXTENT CONTROL')
    print('BANK PASS: 1468 Macintosh/native-original/native-replacement cases;', rejected,
          'rejections; full framebuffer, frame locals, registers, CCR and SP')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('native', type=Path)
    args = parser.parse_args()
    recorded = pins(ROOT/'manifest/bank-skip-evidence.sha256')
    paths = [args.reference/name for name in ('session.log', 'bank.effects')]
    paths += [args.native/name for name in ('receipt.json', 'fixture.elf', 'dh1/WrapperProbe',
              'dh1/original.effects', 'dh1/replacement.effects', 'dh1/fixture.complete',
              'dh1/Shell.done', 'logs/debug.uae')]
    for path in paths:
        key = str(path.resolve().relative_to(ROOT))
        assert key in recorded and hashlib.sha256(path.read_bytes()).hexdigest() == recorded[key], key
    check(args.reference, args.native)
