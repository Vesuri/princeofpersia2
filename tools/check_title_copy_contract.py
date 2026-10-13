"""Verify all original offscreen title copies, including unlocked buffers."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
from extract_original_data import ROOT, pins


def records(reference):
    log = (reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log
    assert log.count('INTEGRATION_COMPLETE calls=65\n') == 1, 'COPY / COMPLETION'
    calls = re.findall(r'^INTEGRATION_CALL id=(\d+) segment=17 offset=508A trap=A8EC$', log, re.M)
    assert calls == [str(i) for i in range(1,66)], 'COPY / COMPLETE CALL SEQUENCE'
    returns = re.findall(r'^INTEGRATION_REGS after id=(\d+) ', log, re.M)
    assert calls == returns, 'COPY / PAIRED RETURNS'
    assert log.count('DISPLAY_ROUTE_CAPTURED first-room\n') == 1, 'COPY / FIRST ROOM'
    digest = hashlib.sha256()
    rows = []
    for ident in calls:
        read = lambda name: (reference/('integration-'+ident+'-'+name+'.bin')).read_bytes()
        stack = read('before-stack')
        assert stack[:6] == bytes(6), 'COPY / MODE AND MASK'
        assert stack[14:18] != stack[18:22], 'COPY / DISTINCT BITMAPS'
        digest.update(stack[:6])
        for name in ('copy-from', 'copy-to'):
            data = read(name)
            assert len(data) == 8, 'COPY / RECT EXTENT'
            digest.update(data)
        expected_locks = (1,2) if ident == '1' else (2,1)
        for side, lock in zip(('Source', 'Destination'), expected_locks):
            before, after = [read(phase+'-copy-'+side+'-map') for phase in ('before', 'after')]
            assert len(before) == 50 and before == after, 'COPY / PIXMAP PRESERVATION'
            assert struct.unpack_from('>H', before, 14)[0] == lock, 'COPY / LOCK STATE'
            # Normalize only live pointers; retain layout, format and lock metadata.
            digest.update(bytes(4)+before[4:42]+bytes(4)+before[46:])
        current = read('before-pixmap')
        assert len(current) == 50, 'COPY / CURRENT PIXMAP EXTENT'
        is_current = current == read('before-copy-Destination-map')
        assert is_current == (ident != '1'), 'COPY / CURRENT DESTINATION STATE'
        port = read('before-port')
        assert len(port) == 108, 'COPY / CURRENT PORT CAPTURE'
        digest.update(current[4:42])
        digest.update(port[16:24])
        if is_current:
            for region in ('before-vis', 'before-clip'):
                data = read(region)
                assert len(data) == 10 and data[:2] == b'\x00\x0a', 'COPY / RECTANGULAR PORT REGION'
                digest.update(data)
        assert read('before-copy-Source-pixels') == read('after-copy-Source-pixels'), 'COPY / SOURCE PRESERVATION'
        for name in ('before-copy-Source-pixels', 'before-copy-Destination-pixels', 'after-copy-Destination-pixels'):
            data = read(name)
            assert len(data) == 528*384, 'COPY / PIXEL EXTENT'
            digest.update(data)
        rows.append(ident)
    return rows, digest.hexdigest()


def check(reference):
    rows, digest = records(reference)
    assert digest == pins(ROOT/'manifest/integration-title-copy-reference.sha256')['title-copy-series.bin'], 'COPY / REFERENCE PIN'
    scratch = Path(tempfile.mkdtemp(prefix='title-copy-contract-', dir=ROOT/'tmp'))
    probe = scratch/'probe'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-fsanitize=address,undefined',
                    '-Isrc', 'tools/probe_title_copy.cpp', '-o', str(probe)], cwd=ROOT, check=True)
    for ident in rows:
        actual = subprocess.check_output([str(probe), str(reference.resolve()), ident])
        expected = (reference/f'integration-{ident}-after-copy-Destination-pixels.bin').read_bytes()
        assert actual == expected, 'COPY / COMPLETE DESTINATION PIXELS '+ident
    print('TITLE COPY CONTRACT PASS: 65 complete pixel results, preserved PixMaps/source, unlocked destination and source, current and noncurrent destinations')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    check(parser.parse_args().reference)
