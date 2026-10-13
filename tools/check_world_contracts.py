"""Compare reached GWorld records with the original title-to-room route."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from extract_original_data import ROOT, pins


def normalized(data, offsets):
    data = bytearray(data)
    for at in offsets:
        assert int.from_bytes(data[at:at+4], 'big'), 'WORLD / NULL POINTER'
        data[at:at+4] = bytes(4)
    return bytes(data)


def records(reference):
    log = (reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log
    assert log.count('INTEGRATION_COMPLETE calls=23\n') == 1, 'WORLD / COMPLETION'
    assert log.count('DISPLAY_ROUTE_CAPTURED first-room\n') == 1, 'WORLD / FIRST ROOM'
    ids = re.findall(r'^INTEGRATION_CALL id=(\d+) segment=17 offset=4E1A trap=AB1D$', log, re.M)
    assert len(ids) == 23, 'WORLD / CALL COUNT'
    result = []
    for ident in ids:
        read = lambda name: (reference/f'integration-{ident}-{name}.bin').read_bytes()
        stack = read('before-stack')
        assert struct.unpack_from('>I', stack)[0] == 2, 'WORLD / FLAGS'
        assert struct.unpack_from('>I', stack, 8)[0] == 0, 'WORLD / EXPLICIT CLUT'
        assert struct.unpack_from('>H', stack, 16)[0] == 8, 'WORLD / DEPTH'
        bounds = read('world-bounds')
        port = read('world-port')
        pixmap = read('world-map')
        assert len(bounds) == 8 and len(port) == 108 and len(pixmap) == 50, 'WORLD / RECORD EXTENT'
        assert pixmap[6:14] == bounds and port[16:24] == bounds, 'WORLD / ORIGINAL BOUNDS'
        result.append((bounds, normalized(port, (2, 8, 24, 28, 32, 58, 62)), normalized(pixmap, (0, 42))))
    return result


def check(reference):
    rows = records(reference)
    canonical = b''.join(b''.join(row) for row in rows)
    expected = pins(ROOT/'manifest/integration-world-reference.sha256')['world-contracts.bin']
    assert hashlib.sha256(canonical).hexdigest() == expected, 'WORLD / ORIGINAL PIN'
    scratch = Path(tempfile.mkdtemp(prefix='world-contract-', dir=ROOT/'tmp'))
    probe = scratch/'probe'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsanitize=address,undefined',
                    '-Isrc', 'tools/probe_gworld_records.cpp', '-o', str(probe)], cwd=ROOT, check=True)
    text = ''.join(' '.join(map(str, struct.unpack('>4h', row[0])))+'\n' for row in rows)
    output = subprocess.check_output([str(probe)], input=text.encode())
    assert len(output) == len(rows)*158, 'WORLD / NATIVE RECORD COUNT'
    for index, (_, port, pixmap) in enumerate(rows):
        native = output[index*158:(index+1)*158]
        assert normalized(native[:108], (2, 8, 24, 28, 32, 58, 62)) == port, 'WORLD / PORT'
        assert normalized(native[108:], (0, 42)) == pixmap, 'WORLD / PIXMAP'
    print('WORLD CONTRACT PASS: 23 original allocations, including 510x384 title buffer; complete public port and PixMap records')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    check(parser.parse_args().reference)
